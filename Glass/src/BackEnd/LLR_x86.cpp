#include "pch.h"

#include "BackEnd/LLR_x86.h"
#include "BackEnd/TypeSystem.h"

#include "math.h"

namespace Glass
{
	static const std::map<IRNodeType, X86_ASM> op_to_set_inst = {
		{IRNodeType::GreaterThan,	X86_SETG},
		{IRNodeType::LesserThan,	X86_SETL},
		{IRNodeType::Equal,			X86_SETE},
		{IRNodeType::NotEqual,		X86_SETNE},
	};

	static const std::map<RegisterUsage, std::vector<X86_Register>> registers = {
		{REG_I8,	{ SIL, DIL, BL, 		R10b, R11b, R12b, R13b, R14b, R15b, R8b, R9b,	AL,  CL,  DL, }},
		{REG_I16,	{ SI,  DI,  BX, 		R10w, R11w, R12w, R13w, R14w, R15w, R8w, R9w,	AX,  CX,  DX, }},
		{REG_I32,	{ ESI, EDI, EBX, 		R10d, R11d, R12d, R13d, R14d, R15d, R8d, R9d,	EAX, ECX, EDX, }},
		{REG_I64,	{ RSI, RDI, RBX,		R10, R11, R12, R13, R14, R15, R8, R9,			RAX, RCX, RDX, }},
	};

	static const std::map<X86_Register, X86_REG_Overlap> register_overlap = {
		{RAX, X86_AX},
		{EAX, X86_AX},
		{AX, X86_AX},
		{AL, X86_AX},

		{RBX, X86_BX},
		{EBX, X86_BX},
		{BX,  X86_BX},
		{BL,  X86_BX},

		{RCX, X86_CX},
		{ECX, X86_CX},
		{CX,  X86_CX},
		{CL,  X86_CX},

		{RDX, X86_DX},
		{EDX, X86_DX},
		{DX, X86_DX},
		{DL, X86_DX},

		{RSI, X86_SI},
		{ESI, X86_SI},
		{SI, X86_SI},
		{SIL, X86_SI},

		{RDI, X86_DI},
		{EDI, X86_DI},
		{DI, X86_SI},
		{DIL, X86_SI},

		{R8, X86_R8},
		{R9, X86_R9},
		{R10, X86_R10},
		{R11, X86_R11},
		{R11, X86_R12},
		{R12, X86_R12},
		{R13, X86_R13},
		{R15, X86_R14},
		{R14, X86_R15},

		{R8d,	X86_R8},
		{R9d,	X86_R9},
		{R10d,	X86_R10},
		{R11d,	X86_R11},
		{R11d,	X86_R12},
		{R12d,	X86_R12},
		{R13d,	X86_R13},
		{R15d,	X86_R14},
		{R14d,	X86_R15},

		{R8w, X86_R8},
		{R9w, X86_R9},
		{R10w, X86_R10},
		{R11w, X86_R11},
		{R11w, X86_R12},
		{R12w, X86_R12},
		{R13w, X86_R13},
		{R15w, X86_R14},
		{R14w, X86_R15},

		{R8b, X86_R8},
		{R9b, X86_R9},
		{R10b, X86_R10},
		{R11b, X86_R11},
		{R11b, X86_R12},
		{R12b, X86_R12},
		{R13b, X86_R13},
		{R15b, X86_R14},
		{R14b, X86_R15},
	};

	X86_BackEnd::X86_BackEnd(IRTranslationUnit* translation_unit, MetaData* metadata)
		:m_TranslationUnit(translation_unit), m_Metadata(metadata)
	{
	}

	std::vector<X86_Inst*> X86_BackEnd::Assemble()
	{
		std::vector<X86_Inst*> build_stream;

		for (auto& [Id, func] : m_Metadata->m_Functions) {

			std::string mangled_name;

			if (func.Foreign) {
				mangled_name = func.Symbol.Symbol;
				m_Externals.push_back(mangled_name);
			}
			else {
				if (func.PolyMorphic == false) {
					mangled_name = MangleName(func.Symbol.Symbol, (TSFunc*)func.Signature);
				}
			}

			X86_Inst* label = ASMA(X86_Inst());
			label->type = X86_LABEL_REF;
			label->as.label.name = ASMA(mangled_name)->c_str();

			m_Data.IR_FunctionLabels[Id] = ASMA(*label);
		}


		MemCpy = ASMA(X86_Inst());
		MemCpy->type = X86_LABEL_REF;
		MemCpy->as.label.name = "memcpy";

		for (auto i : m_TranslationUnit->Instructions) {
			if (i->GetType() == IRNodeType::File) {

				IRFile* ir_file = (IRFile*)i;

				for (auto tl_inst : ir_file->Instructions) {
					Assemble(tl_inst, build_stream);
				}
			}
			else {
				Assemble(i, build_stream);
			}
		}

		std::string assem = Print(build_stream);

		std::ofstream asm_out("output.s");


		asm_out << "format MS64 COFF\n";
		asm_out << "public main\n";

		for (auto& ext : m_Externals) {
			asm_out << fmt::format("extrn '{0}' as {0}\n", ext);
		}

		asm_out << "section '.code' code readable executable ;align 16\n";
		asm_out << assem;

		std::string data_section;
		for (auto& [Id, str] : m_DataStrings) {

			std::string_view view = std::string_view((const char*)str.data(), str.size());

			data_section += fmt::format("str{} db ", Id);

			data_section += "'";

			u64 i = 0;
			while (i < view.size()) {
				char c = view[i];

				if (c == '\\') {

					data_section += "'";

					if (view[i + 1] == 'n') {
						data_section += ", 0ah, ";
						i += 2;
						data_section += "'";
						continue;
					}
				}

				data_section += c;

				i++;
			}

			data_section += "'";
			data_section += ", 0\n";
		}

		asm_out << "section '.data' data readable align 16\n";
		asm_out << data_section;

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
		case IRNodeType::Argument:
		{
			AssembleArgument((IRArgumentAllocation*)inst, stream);
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
		case IRNodeType::MemberAccess:
		{
			AssembleMemberAccess((IRMemberAccess*)inst, stream);
		}break;
		case IRNodeType::Call:
		{
			AssembleCall((IRFunctionCall*)inst, stream);
		}break;
		case IRNodeType::Return:
		{
			AssembleReturn((IRReturn*)inst, stream);
		}break;
		case IRNodeType::ConstValue:
		{
			AssembleConstValue((IRCONSTValue*)inst, stream);
		}break;
		case IRNodeType::Register:
		{
			AssembleIRRegister((IRRegister*)inst, stream);
		}break;
		case IRNodeType::RegisterValue:
		{
			AssembleIRRegisterValue((IRRegisterValue*)inst, stream);
		}break;
		case IRNodeType::Data: {
			AssembleData((IRData*)inst, stream);
		}break;
		case IRNodeType::DataValue: {
			AssembleDataValue((IRDataValue*)inst, stream);
		}break;
		case IRNodeType::NullPtr: {
			SetRegisterValue(Make_Constant(0));
		}break;
		case IRNodeType::GlobDecl:break;

		case IRNodeType::ADD:
		case IRNodeType::SUB:
		case IRNodeType::MUL:
		case IRNodeType::DIV:
			AssembleBinOp((IRBinOp*)inst, stream);
			break;
		case IRNodeType::GreaterThan:
		case IRNodeType::LesserThan:
		case IRNodeType::Equal:
		case IRNodeType::NotEqual:
			AssembleLogicalOp((IRBinOp*)inst, stream);
			break;
		case IRNodeType::PointerCast: {
			SetRegisterValue(GetIRRegister(((IRPointerCast*)inst)->PointerRegister));
		}
									break;

		default:
			GS_CORE_ASSERT(0, "Unknown IR Instruction");
			break;
		}
	}

	void X86_BackEnd::AssembleData(IRData* inst, std::vector<X86_Inst*>& stream)
	{
		m_DataStrings.push_back({ inst->ID,inst->Data });
	}

	void X86_BackEnd::AssembleDataValue(IRDataValue* inst, std::vector<X86_Inst*>& stream)
	{
		X86_Inst* string_ref_inst = ASMA(X86_Inst());
		string_ref_inst->type = X86_DATA_STR_REF;
		string_ref_inst->as.data_str_ref.string_id = inst->DataID;

		SetRegisterValue(string_ref_inst);
	}

	void X86_BackEnd::AssembleFunction(IRFunction* func, std::vector<X86_Inst*>& stream)
	{
		GS_CORE_ASSERT(!func->Overload);

		m_Metadata->m_CurrentFunction = func->ID;

		FunctionMetadata* metadata = m_Metadata->GetFunctionMetadata(func->ID);
		GS_CORE_ASSERT(metadata);

		m_CurrentFunction = metadata;

		if (metadata->PolyMorphic)
			return;

		Reset_CallStackSize();
		m_Data.CurrentFunction_InputCallStackOffset = 0;

		const char* label_name;
		const char* return_label_name;

		if (metadata->Symbol.Symbol == "main") {
			label_name = "main";
			return_label_name = "main_return";
		}
		else {
			label_name = ASMA(MangleName(metadata->Symbol.Symbol, (TSFunc*)metadata->Signature))->c_str();
			return_label_name = ASMA(std::string(label_name) + "_return")->c_str();
		}

		std::vector<X86_Inst*> body_stream;

		bool has_return_value = TypeSystem::GetVoid() != metadata->ReturnType;

		X86_Inst* return_label = ASMA(X86_Inst());

		if (has_return_value) {
			CurrentReturnTarget = AllocateStack(metadata->ReturnType);

			return_label->type = X86_LABEL;
			return_label->as.label.name = return_label_name;
		}

		X86_Inst* return_label_ref = ASMA(X86_Inst());
		return_label_ref->type = X86_LABEL_REF;
		return_label_ref->as.label.name = return_label_name;

		ReturnJmpTarget = return_label_ref;

		m_Data.CurrentFunction_InputCallStackOffset += 8; // size of rbp

		u32 argument_index = 0;
		for (auto& parm : metadata->Arguments) {
			auto allocation = GetArgumentLocation(parm.Type, argument_index, stream, ARG_DIR_IN);
			CurrentFunctionArgumentAllocations.push_back(allocation);
			argument_index++;
		}

		for (auto i : func->Instructions) {
			Assemble(i, body_stream);
		}

		CurrentFunctionArgumentAllocations.clear();

		X86_Inst* label = ASMA(X86_Inst());
		label->type = X86_LABEL;
		label->as.label.name = label_name;
		stream.push_back(label);

		X86_Inst stack_base_ptr_save;
		stack_base_ptr_save.type = X86_PUSH;
		stack_base_ptr_save.as.push.source = Make_Register(RBP);
		stream.push_back(ASMA(stack_base_ptr_save));

		X86_Inst s_ptr_move = {};
		s_ptr_move.type = X86_MOV;
		s_ptr_move.as.move.source = Make_Register(RSP);
		s_ptr_move.as.move.destination = Make_Register(RBP);
		stream.push_back(ASMA(s_ptr_move));

		const u64 stack_alignment = 16;

		u64 aligned_frame_size = m_Data.CurrentFunction_StackFrameSize;
		aligned_frame_size += m_Data.CurrentFunction_CallStackSize ? m_Data.CurrentFunction_CallStackSize : 32;

		aligned_frame_size += (stack_alignment - (aligned_frame_size % stack_alignment)) % stack_alignment;

		//if (aligned_frame_size) {
		X86_Inst prologue;
		prologue.type = X86_ISUB;
		prologue.as.bin_op.destination = Make_Register(RSP);
		prologue.as.bin_op.value = Make_Constant((i64)aligned_frame_size);
		stream.push_back(ASMA(prologue));
		//}

		for (auto body_inst : body_stream) {
			stream.push_back(body_inst);
		}

		if (has_return_value) {
			stream.push_back(return_label);

			X86_Inst* return_move = ASMA(X86_Inst());
			return_move->type = X86_MOV;
			return_move->as.move.destination = GetReturnLocation(metadata->ReturnType, stream);
			return_move->as.move.source = CurrentReturnTarget;
			stream.push_back(return_move);
		}


		//if (aligned_frame_size) {
		X86_Inst epilogue;
		epilogue.type = X86_IADD;
		epilogue.as.bin_op.destination = Make_Register(RSP);
		epilogue.as.bin_op.value = Make_Constant((i64)aligned_frame_size);
		stream.push_back(ASMA(epilogue));
		//}

		X86_Inst stack_base_ptr_pull;
		stack_base_ptr_pull.type = X86_POP;
		stack_base_ptr_pull.as.push.source = Make_Register(RBP);
		stream.push_back(ASMA(stack_base_ptr_pull));

		X86_Inst ret;
		ret.type = X86_RET;

		stream.push_back(ASMA(ret));

		m_Data.CurrentFunction_StackFrameSize = 0;
		CalledVariadicFunction = false;
		Free_All_Register();
		CurrentReturnTarget = nullptr;
		ReturnJmpTarget = nullptr;
	}

	void X86_BackEnd::AssembleConstValue(IRCONSTValue* inst, std::vector<X86_Inst*>& stream)
	{
		i64 data = 0;
		memcpy(&data, &inst->Data, sizeof(i64));
		SetRegisterValue(Make_Constant(data));
	}

	void X86_BackEnd::AssembleAlloca(IRAlloca* inst, std::vector<X86_Inst*>& stream)
	{
		u64 allocation_size = TypeSystem::GetTypeSize(inst->Type);

		m_Data.CurrentFunction_StackFrameSize += allocation_size;

		X86_Inst stack_frame_offset = {};
		stack_frame_offset.type = X86_CONSTANT_OFFSET;
		stack_frame_offset.as.constant_offset.from = Make_Register(RBP);
		stack_frame_offset.as.constant_offset.offset = Make_Constant((i64)m_Data.CurrentFunction_StackFrameSize);
		stack_frame_offset.as.constant_offset.size = InWords(inst->Type);

		if (inst->VarMetadata) {
			stack_frame_offset.comment = inst->VarMetadata->Name.Symbol.c_str();
		}
		else {
			stack_frame_offset.comment = "unknown variable name alloca";
		}

		SetRegisterValue(ASMA(stack_frame_offset));
	}

	void X86_BackEnd::AssembleStore(IRStore* inst, std::vector<X86_Inst*>& stream)
	{
		GS_CORE_ASSERT(inst->Data->GetType() == IRNodeType::RegisterValue);

		GS_CORE_ASSERT(inst->Type);
		u64 stored_size = TypeSystem::GetTypeSize(inst->Type);

		if (!stored_size) {
			SetRegisterValue(ASMA(X86_Inst()));
			return;
		}

		IRRegisterValue* data_as_register_ref = ((IRRegisterValue*)inst->Data);

		if (stored_size > 8) {

			if (IsRegisterValue) {
				SetRegisterValue(ASMA(X86_Inst()));
			}

			auto void_ptr_Ty = TypeSystem::GetPtr(TypeSystem::GetVoid(), 1);
			auto size_Ty = TypeSystem::GetPtr(TypeSystem::GetBasic(IR_u64), 1);

			auto sz = GetArgumentLocation(size_Ty, 2, stream, ARG_DIR_OUT);
			auto memcpy_ret = GetReturnLocation(void_ptr_Ty, stream);

			auto src = GetArgumentLocation(void_ptr_Ty, 1, stream, ARG_DIR_OUT);
			auto source = GetIRRegister(((IRRegisterValue*)inst->Data)->RegisterID);

			if (source->type == X86_CONSTANT_OFFSET) {

				X86_Inst* lea = ASMA(X86_Inst());
				lea->type = X86_LEA;
				lea->as.lea.source = source;
				lea->as.lea.destination = src;
				stream.push_back(lea);
			}
			else {
				Make_Move(source, src, stream, void_ptr_Ty);
			}

			auto dest = GetArgumentLocation(void_ptr_Ty, 0, stream, ARG_DIR_OUT);
			auto destination = GetIRRegister(inst->AddressRegister);

			if (destination->type == X86_CONSTANT_OFFSET) {

				X86_Inst* lea = ASMA(X86_Inst());
				lea->type = X86_LEA;
				lea->as.lea.source = destination;
				lea->as.lea.destination = dest;
				stream.push_back(lea);
			}
			else {
				Make_Move(destination, dest, stream, void_ptr_Ty);
			}

			Make_Move(Make_Constant(stored_size), sz, stream, void_ptr_Ty);

			X86_Inst* call = ASMA(X86_Inst());
			call->type = X86_CALL;
			call->as.call.What = MemCpy;
			stream.push_back(call);

			Free_Register(dest->as.reg_alloc.register_allocation_id);
			Free_Register(src->as.reg_alloc.register_allocation_id);
			Free_Register(sz->as.reg_alloc.register_allocation_id);
			Free_Register(memcpy_ret->as.reg_alloc.register_allocation_id);
		}
		else {

			if (IsRegisterValue) {
				SetRegisterValue(ASMA(X86_Inst()));
			}

			auto data_ir_register = m_Metadata->GetRegister(data_as_register_ref->RegisterID);

			IRNodeType node_type = data_ir_register->Value->GetType();

			switch (node_type) {
			case IRNodeType::GreaterThan:
			case IRNodeType::LesserThan:
			case IRNodeType::Equal:
			case IRNodeType::NotEqual: {
				IRBinOp* as_binop = (IRBinOp*)data_ir_register->Value;
				Make_Move(GetIRRegister(data_as_register_ref->RegisterID), GetIRRegister(inst->AddressRegister), stream, 1);
			}
									 break;
			default: {

				X86_Inst move = {};
				move.type = X86_MOV;
				move.as.move.destination = GetIRRegister(inst->AddressRegister);
				move.as.move.source = GetIRRegister(data_as_register_ref->RegisterID);

				stream.push_back(ASMA(move));
			}
			}
		}
	}

	void X86_BackEnd::AssembleLoad(IRLoad* inst, std::vector<X86_Inst*>& stream)
	{
		GS_CORE_ASSERT(inst->Type);
		u64 load_size = TypeSystem::GetTypeSize(inst->Type);

		if (!load_size) {
			SetRegisterValue(ASMA(X86_Inst()));
			return;
		}

		auto loaded_value_loc = GetIRRegister(inst->AddressRegister);

		if (load_size > 8) {

			//we are taking advantage of the fact that register cannot be modified
			//so we just need to take the ptr no need for a useless copy

			if (0)
			{
				auto loaded_ptr_location = Allocate_Register(RegisterUsageBySize(inst->Type), GetRegisterID(), stream);
				SetRegisterValue(loaded_ptr_location);

				X86_Inst* lea = ASMA(X86_Inst());
				lea->type = X86_LEA;
				lea->as.lea.source = loaded_value_loc;
				lea->as.lea.destination = loaded_ptr_location;
				stream.push_back(lea);
			}
			else {
				SetRegisterValue(loaded_value_loc);
			}
		}
		else {

			auto loaded_location = Allocate_Register(RegisterUsageBySize(load_size), GetRegisterID(), stream);
			SetRegisterValue(loaded_location);

			X86_Inst move = {};
			move.type = X86_MOV;
			move.as.move.source = loaded_value_loc;
			move.as.move.destination = loaded_location;
			move.comment = "load";
			stream.push_back(ASMA(move));

			//Make_Move(loaded_value_loc, loaded_location, stream, )
		}
	}

	void X86_BackEnd::AssembleMemberAccess(IRMemberAccess* member_access, std::vector<X86_Inst*>& stream)
	{
		const StructMetadata* struct_metadata = m_Metadata->GetStructMetadata(member_access->StructID);
		GS_CORE_ASSERT(struct_metadata);
		GS_CORE_ASSERT(member_access->MemberID < struct_metadata->Members.size());

		const MemberMetadata& member = struct_metadata->Members[member_access->MemberID];

		X86_Inst* struct_location = GetIRRegister(member_access->ObjectRegister);
		GS_CORE_ASSERT(struct_location->type == X86_CONSTANT_OFFSET);

		X86_Inst* member_location = ASMA(X86_Inst(*struct_location));
		member_location->as.constant_offset.offset = ASMA(*member_location->as.constant_offset.offset);
		member_location->as.constant_offset.from = ASMA(*member_location->as.constant_offset.from);

		i64& offset = *(i64*)(&member_location->as.constant_offset.offset->as.constant.bytes);
		offset -= member.Offset;

		member_location->comment = ASMA("." + member.Name.Symbol)->c_str();

		SetRegisterValue(member_location);
	}

	void X86_BackEnd::AssembleArgument(IRArgumentAllocation* inst, std::vector<X86_Inst*>& stream)
	{
		GS_CORE_ASSERT(m_CurrentFunction);

		auto argument_type = m_CurrentFunction->Arguments[inst->ArgumentIndex].Type;
		GS_CORE_ASSERT(argument_type);

		auto argument_input_location = CurrentFunctionArgumentAllocations[inst->ArgumentIndex];

		auto argument_size = TypeSystem::GetTypeSize(argument_type);

		if (argument_input_location->type == X86_REG_ALLOC && argument_size > 8) {

			auto argument_store_location = AllocateStack(argument_type);
			SetRegisterValue(argument_store_location);

			auto void_ptr_Ty = TypeSystem::GetPtr(TypeSystem::GetVoid(), 1);
			auto size_Ty = TypeSystem::GetPtr(TypeSystem::GetBasic(IR_u64), 1);

			auto memcpy_ret = GetReturnLocation(void_ptr_Ty, stream);
			auto dest = GetArgumentLocation(void_ptr_Ty, 0, stream, ARG_DIR_IN);
			auto source = GetArgumentLocation(void_ptr_Ty, 1, stream, ARG_DIR_IN);
			auto sz = GetArgumentLocation(size_Ty, 2, stream, ARG_DIR_IN);

			Make_Move(argument_input_location, source, stream, void_ptr_Ty);

			X86_Inst* lea = ASMA(X86_Inst());
			lea->type = X86_LEA;
			lea->as.lea.source = argument_store_location;
			lea->as.lea.destination = dest;
			stream.push_back(lea);

			Make_Move(Make_Constant(argument_size), sz, stream, void_ptr_Ty);

			X86_Inst* call = ASMA(X86_Inst());
			call->type = X86_CALL;
			call->as.call.What = MemCpy;
			stream.push_back(call);

			Free_Register(dest->as.reg_alloc.register_allocation_id);
			Free_Register(source->as.reg_alloc.register_allocation_id);
			Free_Register(sz->as.reg_alloc.register_allocation_id);
			Free_Register(memcpy_ret->as.reg_alloc.register_allocation_id);

			Free_Register(argument_input_location->as.reg_alloc.register_allocation_id);
		}
		else {

			auto argument_store_location = AllocateStack(argument_type);
			SetRegisterValue(argument_store_location);
			Make_Move(argument_input_location, argument_store_location, stream, argument_type, "store argument to stack");

			Free_Register(argument_input_location->as.reg_alloc.register_allocation_id);
		}
	}

	void X86_BackEnd::AssembleCall(IRFunctionCall* inst, std::vector<X86_Inst*>& stream)
	{
		const FunctionMetadata* metadata = m_Metadata->GetFunctionMetadata(inst->FuncID);
		GS_CORE_ASSERT(metadata);

		if (metadata->Variadic) {
			CalledVariadicFunction = true;
		}

		Reset_CallStackPointer();

		X86_Inst* label_ref = m_Data.IR_FunctionLabels.at(inst->FuncID);

		GS_CORE_ASSERT(!inst->Overload, "overloads not supported yet");

		auto return_location = GetReturnLocation(metadata->ReturnType, stream);

		for (size_t i = 0; i < inst->Arguments.size(); i++)
		{
			auto arg = (IRRegisterValue*)inst->Arguments[i];
			GS_CORE_ASSERT(arg->GetType() == IRNodeType::RegisterValue);

			auto arg_type = GetIRRegisterType(arg->RegisterID);

			auto arg_move_destination = GetArgumentLocation(arg_type, i, stream, ARG_DIR_OUT);
			GS_CORE_ASSERT(arg_move_destination);
			auto arg_move_source = GetIRRegister(arg->RegisterID);
			GS_CORE_ASSERT(arg_move_source);

			if (TypeSystem::GetTypeSize(arg_type) > 8) {

				X86_Inst lea = {};
				lea.type = X86_LEA;
				lea.as.lea.source = arg_move_source;
				lea.as.lea.destination = arg_move_destination;
				stream.push_back(ASMA(lea));

				Free_Register(arg_move_destination->as.reg_alloc.register_allocation_id);
			}
			else {
				// 
				// 				X86_Inst move = {};
				// 				move.type = X86_MOV;
				// 				move.as.move.source = arg_move_source;
				// 				move.as.move.destination = arg_move_destination;
				// 				stream.push_back(ASMA(move));

				Make_Move(arg_move_source, arg_move_destination, stream, arg_type, "argument move");

				Free_Register(arg_move_destination->as.reg_alloc.register_allocation_id);
			}
		}

		X86_Inst call = {};
		call.type = X86_CALL;
		call.as.call.What = label_ref;

		stream.push_back(ASMA(call));

		if (IsRegisterValue) {
			SetRegisterValue(return_location);
		}

		// 
		// 		X86_Inst* return_spill = ASMA(X86_Inst());
		// 		return_spill->type = X86_MOV;
		// 		return_spill->as.move.destination = return_stack_loc;
		// 		return_spill->as.move.source = return_location;
		// 		stream.push_back(return_spill);

				//Free_Register(return_location->as.reg_alloc.register_allocation_id);
	}

	void X86_BackEnd::AssembleReturn(IRReturn* inst, std::vector<X86_Inst*>& stream)
	{
		if (CurrentReturnTarget) {

			GS_CORE_ASSERT(inst->Value);
			GS_CORE_ASSERT(inst->Value->GetType() == IRNodeType::RegisterValue);

			Make_Move(GetIRRegister(((IRRegisterValue*)inst->Value)->RegisterID), CurrentReturnTarget, stream, 4);
		}
		else {
			GS_CORE_ASSERT(inst->Value);
		}

		X86_Inst jmp = {};
		jmp.type = X86_JMP;
		jmp.as.jmp.Where = ReturnJmpTarget;
		stream.push_back(ASMA(jmp));
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


		auto temprary_move_a = GetIRRegister(inst->RegisterA->RegisterID);
		auto dest_inst = Allocate_Register(RegisterUsage::REG_I32, GetRegisterID(), stream);

		SetRegisterValue(dest_inst);

		bool move_same = AreEqual(temprary_move_a, dest_inst) && UselessMoveElimination;

		if (!move_same) {
			X86_Inst move = {};
			move.type = X86_MOV;
			move.as.move.source = temprary_move_a;
			move.as.move.destination = dest_inst;
			move.comment = "move first argument argument to add";
			stream.push_back(ASMA(move));
		}

		X86_Inst add;
		add.type = op_lookup.at(node_type);
		add.as.bin_op.destination = dest_inst;
		add.as.bin_op.value = GetIRRegister(inst->RegisterB->RegisterID);
		stream.push_back(ASMA(add));

		return;
	}

	void X86_BackEnd::AssembleLogicalOp(IRBinOp* inst, std::vector<X86_Inst*>& stream)
	{
		auto compare_type = TypeSystem::GetBasic(inst->Type);
		GS_CORE_ASSERT(compare_type);
		auto compare_size = TypeSystem::GetTypeSize(compare_type);
		GS_CORE_ASSERT(compare_size);

		auto register_a = GetIRRegister(inst->RegisterA->RegisterID); // we are freeing before allocation there is a very high chance that we will pick a tmp register that is the same as the operand that will cause for example mov al, al which will be removed if the option is set
		auto destination_register = Allocate_Register(RegisterUsageBySize(compare_type), GetRegisterID(), stream);

		Make_Move(register_a, destination_register, stream, compare_size, "compare move");

		X86_Inst* compare = ASMA(X86_Inst());
		compare->type = X86_CMP;
		compare->as.cmp.a = destination_register;
		compare->as.cmp.b = GetIRRegister(inst->RegisterB->RegisterID);
		Free_Register(destination_register->as.reg_alloc.register_allocation_id);
		stream.push_back(compare);

		auto b8_register = Allocate_Register(REG_I8, GetRegisterID(), stream);

		X86_Inst* conditional_set = ASMA(X86_Inst());
		conditional_set->type = op_to_set_inst.at(inst->GetType());
		conditional_set->as.cond_set.destination = b8_register;
		stream.push_back(conditional_set);

		SetRegisterValue(b8_register);
	}

	void X86_BackEnd::AssembleIRRegister(IRRegister* inst, std::vector<X86_Inst*>& stream)
	{
		IsRegisterValue = true;

		std::vector<X86_Inst*> register_stream;
		Assemble(inst->Value, register_stream);

		GS_CORE_ASSERT(RegisterValue);

		m_Data.IR_RegisterValues[inst->ID] = RegisterValue;
		m_Data.IR_RegisterTypes[inst->ID] = GetIRNodeType(inst->Value);

		for (size_t i = 0; i < register_stream.size(); i++)
		{
			stream.push_back(register_stream[i]);
		}

		RegisterValue = nullptr;
		IsRegisterValue = false;
	}

	void X86_BackEnd::AssembleIRRegisterValue(IRRegisterValue* register_value, std::vector<X86_Inst*>& stream)
	{
		IRRegister* Register = m_Metadata->GetRegister(register_value->RegisterID);

		if (Register->Value->GetType() == IRNodeType::Alloca || Register->Value->GetType() == IRNodeType::Argument || Register->Value->GetType() == IRNodeType::MemberAccess) {

			auto address_register = Allocate_Register(REG_I64, GetRegisterID(), stream);

			Make_LEA(GetIRRegister(register_value->RegisterID), address_register, stream);
			SetRegisterValue(address_register);

			Free_Register(address_register->as.reg_alloc.register_allocation_id);
		}
		else {
			SetRegisterValue(GetIRRegister(register_value->RegisterID));
		}
	}

	TypeStorage* X86_BackEnd::GetIRNodeType(IRInstruction* inst)
	{
		TypeStorage* type = nullptr;
		IRNodeType node_type = inst->GetType();

		switch (node_type)
		{
		case IRNodeType::ConstValue:
		{
			IRCONSTValue* as_const_value = (IRCONSTValue*)inst;
			type = TypeSystem::GetBasic(as_const_value->Type);
		}break;
		case IRNodeType::DataValue:
		{
			type = TypeSystem::GetPtr(TypeSystem::GetBasic(IR_u8), 1);
		}break;
		case IRNodeType::Alloca:
		{
			IRAlloca* as_alloca = (IRAlloca*)inst;
			type = as_alloca->Type;
		}break;
		case IRNodeType::Load:
		{
			IRLoad* as_load = (IRLoad*)inst;
			type = as_load->Type;
		}break;
		case IRNodeType::MemberAccess:
		{
			auto member_access = (IRMemberAccess*)inst;

			const StructMetadata* struct_metadata = m_Metadata->GetStructMetadata(member_access->StructID);
			GS_CORE_ASSERT(struct_metadata);
			GS_CORE_ASSERT(member_access->MemberID < struct_metadata->Members.size());

			const MemberMetadata& member = struct_metadata->Members[member_access->MemberID];
			type = TypeSystem::GetPtr(member.Type, 1);
		}break;
		case IRNodeType::Store:
		{
			IRStore* as_store = (IRStore*)inst;
			type = as_store->Type;
		}break;
		case IRNodeType::Call:
		{
			IRFunctionCall* as_call = (IRFunctionCall*)inst;
			type = m_Metadata->GetFunctionMetadata(as_call->FuncID)->ReturnType;
		}break;
		case IRNodeType::ADD:
		case IRNodeType::SUB:
		case IRNodeType::MUL:
		case IRNodeType::DIV:
		{
			IRBinOp* as_binop = (IRBinOp*)inst;
			type = TypeSystem::GetBasic(as_binop->Type);
		}break;
		case IRNodeType::Argument:
		{
			GS_CORE_ASSERT(m_CurrentFunction);

			IRArgumentAllocation* argument = (IRArgumentAllocation*)inst;
			type = m_CurrentFunction->Arguments[argument->ArgumentIndex].Type;
		}break;
		case IRNodeType::RegisterValue:
		{
			type = m_Data.IR_RegisterTypes.at(((IRRegisterValue*)inst)->RegisterID);
		}break;
		case IRNodeType::NullPtr:
		{
			type = TypeSystem::GetPtr(TypeSystem::GetBasic(((IRNullPtr*)inst)->TypeID), 1);
		}break;
		case IRNodeType::PointerCast:
		{
			type = ((IRPointerCast*)inst)->Type;
		}break;
		case IRNodeType::GreaterThan:
		case IRNodeType::LesserThan:
		case IRNodeType::Equal:
		case IRNodeType::NotEqual:
		{
			//type = TypeSystem::GetBasic(((IRBinOp*)inst)->Type);
		}break;
		default:
			GS_CORE_ASSERT(0);
			break;
		}

		return type;
	}

	std::string X86_BackEnd::Print(const std::vector<X86_Inst*>& assm)
	{
		std::string stream;
		std::string comments;

		for (size_t i = 0; i < assm.size(); i++)
		{
			X86_Inst inst = *assm[i];

			if (inst.type != X86_LABEL) {
				stream += "\t";
			}

			Print(inst, stream, comments);

			stream += comments;

			comments.clear();

			stream += '\n';
		}

		return stream;
	}

	void X86_BackEnd::Print(X86_Inst inst, std::string& stream, std::string& comments)
	{
		switch (inst.type) {
		case X86_LABEL_REF:
			stream += inst.as.label.name;
			break;
		case X86_LABEL:
			stream += inst.as.label.name;
			GS_CORE_ASSERT(inst.as.label.name);
			stream += ":";
			break;

		case X86_CALL:
			stream += "call\t";
			Print(*inst.as.call.What, stream, comments);
			break;

		case X86_JMP:
			stream += "jmp\t";
			Print(*inst.as.call.What, stream, comments);
			break;

		case X86_PUSH:
		case X86_PUSHQ:
			stream += "push";
			stream += "\t\t";
			Print(*inst.as.push.source, stream, comments);
			break;

		case X86_POP:
		case X86_POPQ:
			stream += "pop";
			stream += "\t\t";
			Print(*inst.as.push.source, stream, comments);
			break;

		case X86_MOV:
		case X86_MOVQ:
			stream += "mov";
			stream += "\t\t";
			Print(*inst.as.move.destination, stream, comments);
			stream += ", ";
			Print(*inst.as.move.source, stream, comments);
			break;
		case X86_LEA:
			stream += "lea";
			stream += "\t\t";
			Print(*inst.as.move.destination, stream, comments);
			stream += ", ";
			Print(*inst.as.move.source, stream, comments);
			break;

		case X86_REG_NAME:
			stream += RegisterToString(inst.as.reg_name.Register);
			break;
		case X86_REG_ALLOC:
			stream += RegisterToString(inst.as.reg_alloc.Register);
			break;

		case X86_RET:
			stream += "ret";
			stream += '\n';
			stream += '\n';
			break;

		case X86_ISUB:
			stream += "sub";
			stream += "\t\t";
			Print(*inst.as.bin_op.destination, stream, comments);
			stream += ", ";
			Print(*inst.as.bin_op.value, stream, comments);
			break;
		case X86_IADD:
			stream += "add";
			stream += "\t\t";
			Print(*inst.as.bin_op.destination, stream, comments);
			stream += ", ";
			Print(*inst.as.bin_op.value, stream, comments);
			break;
		case X86_IMUL:
			stream += "imul";
			stream += "\t";
			Print(*inst.as.bin_op.destination, stream, comments);
			stream += ", ";
			Print(*inst.as.bin_op.value, stream, comments);
			break;

		case X86_IDIV:
			stream += "div";
			stream += "\t\t";
			Print(*inst.as.bin_op.destination, stream, comments);
			stream += ", ";
			Print(*inst.as.bin_op.value, stream, comments);
			break;

		case X86_CONSTANT:
			stream += std::to_string(*(i64*)&inst.as.constant.bytes);
			break;

		case X86_CONSTANT_OFFSET: {
			static std::vector<const char*> words = { "", "byte ", "word ", "dword ", "qword " };
			stream += words[inst.as.constant_offset.size];
			stream += "[";
			Print(*inst.as.constant_offset.from, stream, comments);
			stream += inst.as.constant_offset.offset_type ? " + " : " - ";
			Print(*inst.as.constant_offset.offset, stream, comments);
			stream += "]";
			break;
		}

		case X86_DATA_STR_REF: {

			//stream += '[';
			stream += "str";
			stream += std::to_string(inst.as.data_str_ref.string_id);
			//			stream += ']';

			break;
		}

		case X86_CMP:
			stream += "cmp";
			stream += "\t\t";
			Print(*inst.as.cmp.a, stream, comments);
			stream += ", ";
			Print(*inst.as.cmp.b, stream, comments);
			break;

		case X86_SETG:
		case X86_SETGE:
		case X86_SETL:
		case X86_SETLE:
		case X86_SETE:
		case X86_SETNE: {

			static const std::map<X86_ASM, const char*> set_inst_names = {
				{X86_SETG,	"setg"},
				{X86_SETGE, "setge"},
				{X86_SETL,	"setl"},
				{X86_SETLE, "setle"},
				{X86_SETE,	"sete"},
				{X86_SETNE, "setne"},
			};

			stream += set_inst_names.at(inst.type);
			stream += "\t";

			Print(*inst.as.cond_set.destination, stream, comments);

		}
					  break;

		default:
			stream += "un-implemented print inst";
			break;
		}

		if (inst.comment) {
			comments += "\t\t";
			comments += ";";
			comments += inst.comment;
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

			"r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
			"r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d",
			"r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w",
			"r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b",
		};

		return names[reg];
	}

	void X86_BackEnd::Make_MemCpy(u64 source_register_id, u64 destination_register_id, std::vector<X86_Inst*>& stream, TypeStorage* type)
	{
		GS_CORE_ASSERT(type);
		auto cpymem_size = TypeSystem::GetTypeSize(type);

		auto void_ptr_Ty = TypeSystem::GetPtr(TypeSystem::GetVoid(), 1);
		auto size_Ty = TypeSystem::GetPtr(TypeSystem::GetBasic(IR_u64), 1);

		auto src = GetArgumentLocation(void_ptr_Ty, 1, stream, ARG_DIR_OUT);
		auto dest = GetArgumentLocation(void_ptr_Ty, 0, stream, ARG_DIR_OUT);
		auto sz = GetArgumentLocation(size_Ty, 2, stream, ARG_DIR_OUT);
		auto memcpy_ret = GetReturnLocation(void_ptr_Ty, stream); // so that it will not override rax spill it if in use

		Make_Move(GetIRRegister(destination_register_id), dest, stream, void_ptr_Ty);
		Make_Move(GetIRRegister(source_register_id), src, stream, void_ptr_Ty);
		Make_Move(Make_Constant(cpymem_size), sz, stream, void_ptr_Ty);

		X86_Inst* call = ASMA(X86_Inst());
		call->type = X86_CALL;
		call->as.call.What = MemCpy;
		stream.push_back(call);

		Free_Register(dest->as.reg_alloc.register_allocation_id);
		Free_Register(src->as.reg_alloc.register_allocation_id);
		Free_Register(sz->as.reg_alloc.register_allocation_id);
		Free_Register(memcpy_ret->as.reg_alloc.register_allocation_id);
	}

	void X86_BackEnd::Make_LocalStack_MemCpy(X86_Inst* source_stack_offset, X86_Inst* destination_stack_offset, std::vector<X86_Inst*>& stream, TypeStorage* type)
	{
		GS_CORE_ASSERT(type);
		auto cpymem_size = TypeSystem::GetTypeSize(type);

		auto void_ptr_Ty = TypeSystem::GetPtr(TypeSystem::GetVoid(), 1);
		auto size_Ty = TypeSystem::GetPtr(TypeSystem::GetBasic(IR_u64), 1);

		auto dest = GetArgumentLocation(void_ptr_Ty, 0, stream, ARG_DIR_OUT);
		auto src = GetArgumentLocation(void_ptr_Ty, 1, stream, ARG_DIR_OUT);
		auto sz = GetArgumentLocation(size_Ty, 2, stream, ARG_DIR_OUT);
		auto memcpy_ret = GetReturnLocation(void_ptr_Ty, stream); // so that it will not override rax, will spill it if in use

		Make_LEA(destination_stack_offset, dest, stream);
		Make_LEA(source_stack_offset, src, stream);
		Make_Move(Make_Constant(cpymem_size), sz, stream, size_Ty);

		X86_Inst* call = ASMA(X86_Inst());
		call->type = X86_CALL;
		call->as.call.What = MemCpy;
		stream.push_back(call);

		Free_Register(dest->as.reg_alloc.register_allocation_id);
		Free_Register(src->as.reg_alloc.register_allocation_id);
		Free_Register(sz->as.reg_alloc.register_allocation_id);
		Free_Register(memcpy_ret->as.reg_alloc.register_allocation_id);
	}

	X86_Inst* X86_BackEnd::Make_Move(X86_Inst* source, X86_Inst* destination, std::vector<X86_Inst*>& intermediate_stream, TypeStorage* type, const char* comment)
	{
		return Make_Move(source, destination, intermediate_stream, TypeSystem::GetTypeSize(type), comment);
	}

	X86_Inst* X86_BackEnd::Make_Move(X86_Inst* source, X86_Inst* destination, std::vector<X86_Inst*>& intermediate_stream, u64 size, const char* comment)
	{

		if (source->type == X86_CONSTANT_OFFSET && destination->type == X86_CONSTANT_OFFSET) { //	needs intermediate register

			auto dest_register = Allocate_Register(RegisterUsageBySize(size), GetRegisterID(), intermediate_stream);

			X86_Inst intermediate = {};
			intermediate.type = X86_MOV;
			intermediate.as.move.source = source;
			intermediate.as.move.destination = dest_register;
			intermediate.comment = "intermediate move to register";
			source = dest_register;
			intermediate_stream.push_back(ASMA(intermediate));

			Free_Register(dest_register->as.reg_alloc.register_allocation_id);
		}

		bool useless_move = AreEqual(source, destination);

		if (!useless_move) {

			X86_Inst* move = ASMA(X86_Inst());
			move->type = X86_MOV;
			move->as.move.source = source;
			move->as.move.destination = destination;
			move->comment = comment;

			intermediate_stream.push_back(move);
		}

		return destination;
	}

	void X86_BackEnd::Make_LEA(X86_Inst* source, X86_Inst* destination, std::vector<X86_Inst*>& stream)
	{
		X86_Inst* lea = ASMA(X86_Inst());
		lea->type = X86_LEA;
		lea->as.lea.source = source;
		lea->as.lea.destination = destination;

		stream.push_back(lea);
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

	TypeStorage* X86_BackEnd::GetIRRegisterType(u64 id)
	{
		return m_Data.IR_RegisterTypes.at(id);
	}

	RegisterUsage X86_BackEnd::RegisterUsageBySize(TypeStorage* type)
	{
		GS_CORE_ASSERT(type);
		auto type_size = TypeSystem::GetTypeSize(type);
		return RegisterUsageBySize(type_size);
	}

	RegisterUsage X86_BackEnd::RegisterUsageBySize(u64 type_size)
	{
		GS_CORE_ASSERT(type_size);

		if (type_size > 4) {
			return RegisterUsage::REG_I64;
		}
		else if (type_size == 4) {
			return RegisterUsage::REG_I32;
		}
		else if (type_size == 2) {
			return RegisterUsage::REG_I16;
		}
		else if (type_size == 1) {
			return RegisterUsage::REG_I8;
		}
	}

	X86_Word X86_BackEnd::InWords(TypeStorage* type)
	{
		static const std::map<u64, X86_Word> size_table = {
			{1,X86_byte}, {2,X86_word}, {4,X86_dword}, {8,X86_qword},
		};

		GS_CORE_ASSERT(type);
		auto type_size = TypeSystem::GetTypeSize(type);

		if (type_size <= 8) {
			return size_table.at(type_size);
		}
		else {
			return (X86_Word)0;
		}
	}

	X86_Word X86_BackEnd::InWords(u64 type_size)
	{
		static const std::map<u64, X86_Word> size_table = {
			{1,X86_byte}, {2,X86_word}, {4,X86_dword}, {8,X86_qword},
		};

		GS_CORE_ASSERT(type_size);

		if (type_size <= 8) {
			return size_table.at(type_size);
		}
		else {
			return (X86_Word)0;
		}
	}

	u64 X86_BackEnd::RegiserSize(X86_Register reg)
	{
		static const std::map<X86_Register, u64> sizes = {
			{RSP,8},{RBP,8},{RAX,8},{RBX,8},{RCX,8},{RDX,8},{RSI,8},{RDI,8},
			{R8,8},{R9,8},{R10,8},{R11,8},{R12,8},{R13,8},{R14,8},{R15,8},
			{ESP,4},{EBP,4},{EAX,4},{EBX,4},{ECX,4},{EDX,4},{ESI,4},{EDI,4},
			{R8d,4},{R9d,4},{R10d,4},{R11d,4},{R12d,4},{R13d,4},{R14d,4},{R15d,4},
		};

		return sizes.at(reg);
	}

	void X86_BackEnd::Free_Register(u32 id)
	{
		if (RegisterAllocationIDs.find(id) == RegisterAllocationIDs.end()) {
			return;
		}

		auto phys_reg = RegisterAllocationIDs.at(id);
		auto overlap = register_overlap.at(phys_reg->as.reg_alloc.Register);
		RegisterOccupations[overlap] = false;

		//RegisterAllocationIDs[id]->as.reg_alloc.free_after_use = false;
		//RegisterAllocationIDs[id]->as.reg_alloc.Register = (X86_Register)999;
	}

	void X86_BackEnd::Free_All_Register()
	{
		RegisterOccupations.clear();
		RegisterAllocationIDs.clear();
	}

	X86_Inst* X86_BackEnd::Allocate_Register(RegisterUsage usage, u32 id, std::vector<X86_Inst*>& spillage_stream)
	{
		static const std::map<RegisterUsage, TypeStorage*> UsageToType = {
			{RegisterUsage::REG_I8, TypeSystem::GetBasic(IR_u8)},
			{RegisterUsage::REG_I16, TypeSystem::GetBasic(IR_u16)},
			{RegisterUsage::REG_I32, TypeSystem::GetBasic(IR_u32)},
			{RegisterUsage::REG_I64, TypeSystem::GetBasic(IR_u64)}
		};

		const auto& slice = registers.at(usage);

		for (auto phys_reg : slice) {

			auto overlap = register_overlap.at(phys_reg);

			if (!RegisterOccupations[overlap]) {

				X86_Inst* allocation_inst = ASMA(X86_Inst());
				allocation_inst->type = X86_REG_ALLOC;
				allocation_inst->as.reg_alloc.Register = phys_reg;
				allocation_inst->as.reg_alloc.register_allocation_id = id;
				allocation_inst->as.reg_alloc.free_after_use = true;

				RegisterAllocations[overlap] = allocation_inst;
				RegisterAllocationIDs[id] = allocation_inst;
				RegisterOccupations[overlap] = true;
				RegisterAllocationTypes[overlap] = UsageToType.at(usage);

				return allocation_inst;
			}
		}

		auto overlap = register_overlap.at(slice[0]);

		RegisterOccupations.at(overlap) = false;
		X86_Inst* previous_allocation = RegisterAllocations.at(overlap);

		auto allocation_type = RegisterAllocationTypes.at(overlap);

		auto spillage_location = AllocateStack(allocation_type);

		Make_Move(ASMA(X86_Inst(*previous_allocation)), spillage_location, spillage_stream, allocation_type, "spillage");
		*previous_allocation = *spillage_location;

		return Allocate_Register(usage, id, spillage_stream);
	}

	X86_Inst* X86_BackEnd::Allocate_Specific_Register(X86_Register reg, u32 id, std::vector<X86_Inst*>& spillage_stream)
	{
		auto overlap = register_overlap.at(reg);

		if (!RegisterOccupations[overlap]) {

			X86_Inst* allocation_inst = ASMA(X86_Inst());
			allocation_inst->type = X86_REG_ALLOC;
			allocation_inst->as.reg_alloc.Register = reg;
			allocation_inst->as.reg_alloc.register_allocation_id = id;
			allocation_inst->as.reg_alloc.free_after_use = true;

			RegisterAllocations[overlap] = allocation_inst;
			RegisterAllocationIDs[id] = allocation_inst;
			RegisterOccupations[overlap] = true;

			return allocation_inst;
		}
		else {
			RegisterOccupations[overlap] = false;
			X86_Inst* previous_allocation = RegisterAllocations[overlap];

			auto allocation_size = RegiserSize(previous_allocation->as.reg_alloc.Register);

			auto spillage_location = AllocateStack(allocation_size);

			Make_Move(ASMA(X86_Inst(*previous_allocation)), spillage_location, spillage_stream, allocation_size, "spillage");
			*previous_allocation = *spillage_location;

			return Allocate_Specific_Register(reg, id, spillage_stream);
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

	X86_Inst* X86_BackEnd::AllocateStack(TypeStorage* type)
	{
		u64 allocation_size = TypeSystem::GetTypeSize(type);
		return AllocateStack(allocation_size);
	}

	X86_Inst* X86_BackEnd::AllocateStack(u64 allocation_size)
	{
		m_Data.CurrentFunction_StackFrameSize += allocation_size;

		X86_Inst stack_frame_offset = {};
		stack_frame_offset.type = X86_CONSTANT_OFFSET;
		stack_frame_offset.as.constant_offset.from = Make_Register(RBP);
		stack_frame_offset.as.constant_offset.offset = Make_Constant((i64)m_Data.CurrentFunction_StackFrameSize);
		stack_frame_offset.as.constant_offset.size = InWords(allocation_size);
		return ASMA(stack_frame_offset);
	}

	X86_Inst* X86_BackEnd::Allocate_CallStack(u64 allocation_size)
	{
		if (m_Data.CurrentFunction_CallStackSize == 0) {
			m_Data.CurrentFunction_CallStackSize = 32; // i dont know why on windows arguments passed through the stack has to be 32 bytes offseted from stack top 
			m_Data.CurrentFunction_CallStackPointer = 32;
		}

		X86_Inst* stack_top_offset = ASMA(X86_Inst());
		stack_top_offset->type = X86_CONSTANT_OFFSET;
		stack_top_offset->as.constant_offset.from = Make_Register(RSP);
		stack_top_offset->as.constant_offset.offset = Make_Constant(m_Data.CurrentFunction_CallStackPointer);
		stack_top_offset->as.constant_offset.offset_type = X86_CONSTANT_ADD;
		stack_top_offset->as.constant_offset.size = InWords(allocation_size);

		m_Data.CurrentFunction_CallStackPointer += (allocation_size < 8 ? 8 : allocation_size);
		if (m_Data.CurrentFunction_CallStackPointer >= m_Data.CurrentFunction_CallStackSize) {
			m_Data.CurrentFunction_CallStackSize = m_Data.CurrentFunction_CallStackPointer;
		}

		return stack_top_offset;
	}

	void X86_BackEnd::Reset_CallStackPointer()
	{
		m_Data.CurrentFunction_CallStackPointer = 32;
	}

	void X86_BackEnd::Reset_CallStackSize()
	{
		m_Data.CurrentFunction_CallStackSize = 0;
	}

	u32 X86_BackEnd::GetRegisterID()
	{
		return RegisterIDCounter++;
	}

	X86_Inst* X86_BackEnd::GetArgumentLocation(TypeStorage* type, u32 index, std::vector<X86_Inst*>& spillage_stream, GetArgument_Location_Dir direction)
	{
		//C Decl Calling Convention On X86_64

		static const std::map<std::pair<u32, u32>, X86_Register> scalar_registers = {
			//index, size, reg
			{	{0,	 8},	RCX},
			{	{1,	 8},	RDX},

			{	{0,	 4},	ECX},
			{	{1,	 4},	EDX},

			{	{0,	 2},	CX},
			{	{1,	 2},	DX},

			{	{0,	 1},	CL},
			{	{1,	 1},	DL},

			{	{2,	 8},	R8},
			{	{3,	 8},	R9},

			{	{2,	 4},	R8d},
			{	{3,	 4},	R9d},
		};

		u32 type_size = TypeSystem::GetTypeSize(type);

		if (index < 4) {
			if (type_size <= 8) {
				auto needed_register = scalar_registers.at({ index, type_size });
				return Allocate_Specific_Register(needed_register, GetRegisterID(), spillage_stream);
			}
			else {
				auto ptr_size = TypeSystem::GetTypeSize(TypeSystem::GetPtr(TypeSystem::GetVoid(), 1));
				auto needed_register = scalar_registers.at({ index, (u32)ptr_size });
				return Allocate_Specific_Register(needed_register, GetRegisterID(), spillage_stream);
			}
		}
		else {
			if (direction == ARG_DIR_IN) {

				if (m_Data.CurrentFunction_InputCallStackPointer == 0) {
					m_Data.CurrentFunction_InputCallStackPointer = m_Data.CurrentFunction_InputCallStackOffset + 32; // 32 magic number on windows
				}

				m_Data.CurrentFunction_InputCallStackPointer += type_size < 8 ? 8 : type_size;

				X86_Inst* caller_stack_top_offset = ASMA(X86_Inst());
				caller_stack_top_offset->type = X86_CONSTANT_OFFSET;
				caller_stack_top_offset->as.constant_offset.from = Make_Register(RBP);
				caller_stack_top_offset->as.constant_offset.offset = Make_Constant(m_Data.CurrentFunction_InputCallStackPointer);
				caller_stack_top_offset->as.constant_offset.offset_type = X86_CONSTANT_ADD;
				caller_stack_top_offset->as.constant_offset.size = InWords(type);

				return caller_stack_top_offset;

			}
			else {
				return Allocate_CallStack(type_size);
			}
		}
	}

	X86_Inst* X86_BackEnd::GetReturnLocation(TypeStorage* type, std::vector<X86_Inst*>& spillage_stream)
	{
		u64 type_size = TypeSystem::GetTypeSize(type);

		if (type_size > 4) {
			return Allocate_Specific_Register(RAX, GetRegisterID(), spillage_stream);
		}
		else {
			return Allocate_Specific_Register(EAX, GetRegisterID(), spillage_stream);
		}
	}
}