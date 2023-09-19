#include "pch.h"

#include "BackEnd/LLR_x86.h"
#include "BackEnd/TypeSystem.h"

namespace Glass
{

	static const std::map<RegisterUsage, std::vector<X86_Register>> registers = {
		{REG_I32, { EAX, ECX, EDX, EBX, ESI, EDI}},
		{REG_I64, { RAX, RCX, RDX, RBX, RSI, RDI,}},
	};

	static const std::map<X86_Register, X86_REG_Overlap> register_overlap = {
		{RAX, X86_AX},
		{EAX, X86_AX},

		{RBX, X86_BX},
		{EBX, X86_BX},

		{RCX, X86_CX},
		{ECX, X86_CX},

		{RDX, X86_DX},
		{EDX, X86_DX},

		{RSI, X86_SI},
		{ESI, X86_SI},

		{RDI, X86_DI},
		{EDI, X86_DI},
	};

	X86_BackEnd::X86_BackEnd(IRTranslationUnit* translation_unit, MetaData* metadata)
		:m_TranslationUnit(translation_unit), m_Metadata(metadata)
	{
	}

	std::vector<X86_Inst*> X86_BackEnd::Assemble()
	{
		std::vector<X86_Inst*> build_stream;

		for (auto i : m_TranslationUnit->Instructions) {
			if (i->GetType() == IRNodeType::File) {

				IRFile* ir_file = (IRFile*)i;

				for (auto tl_inst : ir_file->Instructions) {
					if (tl_inst->GetType() == IRNodeType::Function) {
						Assemble((IRFunction*)tl_inst, build_stream);
					}
				}
			}
		}

		std::string assem = Print(build_stream);

		std::ofstream asm_out("output.s");

		asm_out <<
			R"(
format MS64 COFF
public main
section '.data' data readable writeable align 16

section '.code' code readable executable ;align 16
extrn 'printf' as _printf
)";

		asm_out << assem;

		GS_CORE_TRACE("ASM:\n{}", assem);

		return build_stream;
	}

	void X86_BackEnd::Assemble(IRInstruction* inst, std::vector<X86_Inst*>& stream)
	{
		IRNodeType node_type = inst->GetType();
		switch (node_type)
		{
		case IRNodeType::Function:
		{
			AssembleFunction((IRFunction*)inst, stream);
		}break;
		case IRNodeType::Alloca:
		{
			AssembleAlloca((IRAlloca*)inst, stream);
		}break;
		case IRNodeType::Store:
		{
			AssembleStore((IRStore*)inst, stream);
		}break;
		case IRNodeType::Load:
		{
			AssembleLoad((IRLoad*)inst, stream);
		}break;
		case IRNodeType::ConstValue:
		{
			AssembleConstValue((IRCONSTValue*)inst, stream);
		}break;
		case IRNodeType::SSA:
		{
			AssembleIRRegister((IRSSA*)inst, stream);
		}break;
		case IRNodeType::Data:break;
		case IRNodeType::GlobDecl:break;

		case IRNodeType::ADD:
		case IRNodeType::SUB:
		case IRNodeType::MUL:
		case IRNodeType::DIV:
			AssembleBinOp((IRBinOp*)inst, stream);
			break;

		default:
			GS_CORE_ASSERT(0, "Unknown IR Instruction");
			break;
		}
	}

	void X86_BackEnd::AssembleFunction(IRFunction* inst, std::vector<X86_Inst*>& stream)
	{
		FunctionMetadata* metadata = m_Metadata->GetFunctionMetadata(inst->ID);
		GS_CORE_ASSERT(metadata);

		if (metadata->PolyMorphic)
			return;

		const char* label_name;

		std::vector<X86_Inst*> body_stream;

		for (auto i : inst->Instructions) {
			Assemble(i, body_stream);
		}

		if (metadata->Symbol.Symbol == "main") {
			label_name = "main";
		}
		else {
			label_name = ASMA(MangleName(metadata->Symbol.Symbol, (TSFunc*)metadata->Signature))->c_str();
		}

		X86_Inst label;
		label.type = X86_LABEL;
		label.as.label.name = label_name;
		stream.push_back(ASMA(label));

		X86_Inst stack_base_ptr_save;
		stack_base_ptr_save.type = X86_PUSH;
		stack_base_ptr_save.as.push.source = Make_Register(RBP);
		stream.push_back(ASMA(stack_base_ptr_save));

		const u64 stack_alignment = 16;
		u64 aligned_frame_size = m_Data.CurrentFunction_StackFrameSize;
		aligned_frame_size += (stack_alignment - (m_Data.CurrentFunction_StackFrameSize % stack_alignment)) % stack_alignment;

		X86_Inst prologue;
		prologue.type = X86_ISUB;
		prologue.as.bin_op.destination = Make_Register(RSP);
		prologue.as.bin_op.value = Make_Constant((i64)aligned_frame_size);
		stream.push_back(ASMA(prologue));

		for (auto body_inst : body_stream) {
			stream.push_back(body_inst);
		}

		X86_Inst epilogue;
		epilogue.type = X86_IADD;
		epilogue.as.bin_op.destination = Make_Register(RSP);
		epilogue.as.bin_op.value = Make_Constant((i64)aligned_frame_size);
		stream.push_back(ASMA(epilogue));

		X86_Inst stack_base_ptr_pull;
		stack_base_ptr_pull.type = X86_POP;
		stack_base_ptr_pull.as.push.source = Make_Register(RBP);
		stream.push_back(ASMA(stack_base_ptr_pull));

		X86_Inst ret;
		ret.type = X86_RET;

		stream.push_back(ASMA(ret));

		m_Data.CurrentFunction_StackFrameSize = 0;
	}

	void X86_BackEnd::AssembleConstValue(IRCONSTValue* inst, std::vector<X86_Inst*>& stream)
	{
		i64 data = 0;
		memcpy(&data, &inst->Data, sizeof(i64));
		stream.push_back(Make_Constant(data));
	}

	void X86_BackEnd::AssembleAlloca(IRAlloca* inst, std::vector<X86_Inst*>& stream)
	{
		u64 allocation_size = TypeSystem::GetTypeSize(inst->Type);

		if (VariableRegisterPromotion && allocation_size <= 4) {
			stream.push_back(Allocate_Register(REG_I32, GetRegisterID()));
			return;
		}

		m_Data.CurrentFunction_StackFrameSize += allocation_size;

		X86_Inst stack_frame_offset = {};
		stack_frame_offset.type = X86_CONSTANT_OFFSET;
		stack_frame_offset.as.constant_offset.from = Make_Register(RSP);
		stack_frame_offset.as.constant_offset.offset = Make_Constant((i64)m_Data.CurrentFunction_StackFrameSize);
		stack_frame_offset.as.constant_offset.size = dword;
		stream.push_back(ASMA(stack_frame_offset));
	}

	void X86_BackEnd::AssembleStore(IRStore* inst, std::vector<X86_Inst*>& stream)
	{
		GS_CORE_ASSERT(inst->Data->GetType() == IRNodeType::SSAValue);

		X86_Inst move = {};
		move.type = X86_MOV;
		move.as.move.destination = GetIRRegister(inst->AddressSSA);
		move.as.move.source = GetIRRegister(((IRSSAValue*)inst->Data)->SSA);

		stream.push_back(ASMA(move));
	}

	void X86_BackEnd::AssembleLoad(IRLoad* inst, std::vector<X86_Inst*>& stream)
	{
		auto loaded = GetIRRegister(inst->AddressSSA);
		auto tmp_location = Allocate_Register(RegisterUsage::REG_I32, GetRegisterID());

		stream.push_back(tmp_location);

		X86_Inst move = {};
		move.type = X86_MOV;
		move.as.move.source = loaded;
		move.as.move.destination = tmp_location;
		stream.push_back(ASMA(move));
	}

	void X86_BackEnd::AssembleBinOp(IRBinOp* inst, std::vector<X86_Inst*>& stream)
	{
		IRNodeType node_type = inst->GetType();

		static std::map<IRNodeType, X86_ASM> op_lookup = {
			{IRNodeType::ADD,X86_IADD},
			{IRNodeType::SUB,X86_ISUB},

			{IRNodeType::MUL,X86_IMUL},
			{IRNodeType::DIV,X86_IDIV},
		};

		auto temprary_move_a = GetIRRegister(inst->SSA_A->SSA);
		auto dest_inst = Allocate_Register(RegisterUsage::REG_I32, GetRegisterID());

		stream.push_back(dest_inst);

		bool move_same = AreEqual(temprary_move_a, dest_inst) && UselessMoveElimination;

		if (!move_same) {
			X86_Inst move = {};
			move.type = X86_MOV;
			move.as.move.source = temprary_move_a;
			move.as.move.destination = dest_inst;
			stream.push_back(ASMA(move));
		}

		X86_Inst add;
		add.type = op_lookup.at(node_type);
		add.as.bin_op.destination = dest_inst;
		add.as.bin_op.value = GetIRRegister(inst->SSA_B->SSA);
		stream.push_back(ASMA(add));

		return;
	}

	void X86_BackEnd::AssembleIRRegister(IRSSA* inst, std::vector<X86_Inst*>& stream)
	{
		std::vector<X86_Inst*> register_stream;
		Assemble(inst->Value, register_stream);

		if (register_stream.size() > 0) {
			m_Data.IR_RegisterValues[inst->ID] = register_stream[0];
		}

		if (register_stream.size() > 1) {
			for (size_t i = 1; i < register_stream.size(); i++)
			{
				stream.push_back(register_stream[i]);
			}
		}
	}

	std::string X86_BackEnd::Print(const std::vector<X86_Inst*>& assm)
	{
		std::string stream;

		for (size_t i = 0; i < assm.size(); i++)
		{
			X86_Inst inst = *assm[i];

			if (inst.type != X86_LABEL) {
				stream += "\t";
			}

			Print(inst, stream);

			stream += '\n';
		}

		return stream;
	}

	void X86_BackEnd::Print(X86_Inst inst, std::string& stream)
	{
		switch (inst.type) {
		case X86_LABEL:
			stream += inst.as.label.name;
			stream += ":";
			break;

		case X86_PUSH:
		case X86_PUSHQ:
			stream += "push";
			stream += "\t";
			Print(*inst.as.push.source, stream);
			break;

		case X86_POP:
		case X86_POPQ:
			stream += "pop";
			stream += "\t";
			Print(*inst.as.push.source, stream);
			break;

		case X86_MOV:
		case X86_MOVQ:
			stream += "mov";
			stream += "\t";
			Print(*inst.as.move.destination, stream);
			stream += ", ";
			Print(*inst.as.move.source, stream);
			break;

		case X86_REG_NAME:
			stream += RegisterToString(inst.as.reg_name.Register);
			break;
		case X86_REG_ALLOC:
			stream += RegisterToString(inst.as.reg_alloc.Register);
			break;

		case X86_RET:
			stream += "ret";
			break;

		case X86_ISUB:
			stream += "sub";
			stream += "\t";
			Print(*inst.as.bin_op.destination, stream);
			stream += ", ";
			Print(*inst.as.bin_op.value, stream);
			break;
		case X86_IADD:
			stream += "add";
			stream += "\t";
			Print(*inst.as.bin_op.destination, stream);
			stream += ", ";
			Print(*inst.as.bin_op.value, stream);
			break;

		case X86_CONSTANT:
			stream += std::to_string(*(i64*)&inst.as.constant.bytes);
			break;

		case X86_CONSTANT_OFFSET: {
			static std::vector<const char*> words = { "word","dword","qword" };
			stream += words[inst.as.constant_offset.size];
			stream += " [";
			Print(*inst.as.constant_offset.from, stream);
			stream += " - ";
			Print(*inst.as.constant_offset.offset, stream);
			stream += "]";
			break;
		}

		default:
			stream += "un-implemented print inst";
			break;
		}
	}

	std::string X86_BackEnd::MangleName(const std::string& name, TSFunc* signature)
	{
		return fmt::format("{}_{}", name, (void*)signature->Hash);
	}

	std::string X86_BackEnd::RegisterToString(X86_Register reg)
	{
		static std::vector<std::string> names = {
			"rsp", "rbp", "rax", "rcx", "rdx", "rbx", "rsi", "rdi",
			"esp", "ebp", "eax", "ecx", "edx", "ebx", "esi", "edi",
			"sp", "bp", "ax", "cx", "dx", "bx", "si", "di",
			"spl", "bpl", "ah", "al", "ch", "cl", "dh", "dl", "bh", "bl", "sil", "dil",
			"xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7",
		};

		return names[reg];
	}

	X86_Inst* X86_BackEnd::Make_Register(X86_Register register_type)
	{
		X86_Inst* inst = new X86_Inst;
		inst->type = X86_REG_NAME;
		inst->as.reg_name = X86_Reg_Name_Inst{ register_type };

		return inst;
	}

	X86_Inst* X86_BackEnd::Make_Constant(i64 integer)
	{
		X86_Inst* inst = new X86_Inst;

		inst->type = X86_CONSTANT;
		memcpy(&inst->as.constant.bytes, &integer, 8);

		return inst;
	}

	X86_Inst* X86_BackEnd::GetIRRegister(u64 id)
	{
		X86_Inst* ir_register_value = m_Data.IR_RegisterValues.at(id);

		if (ir_register_value->type == X86_REG_ALLOC) {

			if (ir_register_value->as.reg_alloc.free_after_use) {
				Free_Register(ir_register_value->as.reg_alloc.register_allocation_id);
			}
		}

		return ir_register_value;
	}

	void X86_BackEnd::Free_Register(u32 id)
	{
		auto phys_reg = RegisterAllocationIDs.at(id);
		auto overlap = register_overlap.at(phys_reg->as.reg_alloc.Register);
		RegisterOccupations[overlap] = false;

		//RegisterAllocationIDs[id]->as.reg_alloc.free_after_use = false;
		//RegisterAllocationIDs[id]->as.reg_alloc.Register = (X86_Register)999;
	}

	X86_Inst* X86_BackEnd::Allocate_Register(RegisterUsage usage, u32 id)
	{
		const auto& slice = registers.at(usage);

		for (auto phys_reg : slice) {

			auto overlap = register_overlap.at(phys_reg);

			if (!RegisterOccupations[overlap]) {

				X86_Inst* allocation_inst = ASMA(X86_Inst());
				allocation_inst->type = X86_REG_ALLOC;
				allocation_inst->as.reg_alloc.Register = phys_reg;
				allocation_inst->as.reg_alloc.register_allocation_id = id;
				allocation_inst->as.reg_alloc.free_after_use = true;

				RegisterAllocationIDs[id] = allocation_inst;
				RegisterOccupations[overlap] = true;

				return allocation_inst;
			}
		}
	}

	bool X86_BackEnd::AreEqual(X86_Inst* a, X86_Inst* b)
	{
		if (a->type == X86_REG_NAME && b->type == X86_REG_NAME) {
			return a->as.reg_name.Register == b->as.reg_name.Register;
		}
		if (a->type == X86_REG_ALLOC && b->type == X86_REG_ALLOC) {
			return a->as.reg_alloc.Register == b->as.reg_alloc.Register;
		}
		return false;
	}

	u32 X86_BackEnd::GetRegisterID()
	{
		return RegisterIDCounter++;
	}
}