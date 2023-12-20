#include "pch.h"

#include "BackEnd/LLR_x86.h"
#include "BackEnd/TypeSystem.h"

#include "math.h"

namespace Glass
{
	Assembly_Instruction Builder::Ret()
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_Ret;

		return instruction;
	}

	Assembly_Instruction Builder::Push(Assembly_Operand* operand)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_Push;
		instruction.Operand1 = operand;

		return instruction;
	}

	Assembly_Instruction Builder::Pop(Assembly_Operand* operand)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_Pop;
		instruction.Operand1 = operand;

		return instruction;
	}

	Assembly_Instruction Builder::Add(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_Add;
		instruction.Operand1 = operand1;
		instruction.Operand2 = operand2;

		return instruction;
	}

	Assembly_Instruction Builder::Sub(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_Sub;
		instruction.Operand1 = operand1;
		instruction.Operand2 = operand2;

		return instruction;
	}

	Assembly_Instruction Builder::Mov(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_Mov;
		instruction.Operand1 = operand1;
		instruction.Operand2 = operand2;

		return instruction;
	}

	Assembly_Operand* Builder::Register(X86_Register reg)
	{
		Assembly_Operand operand = {};
		operand.reg.Register = reg;
		operand.type = Op_Register;

		return ASMA(operand);
	}

	Assembly_Operand* Builder::Constant_Integer(i64 integer)
	{
		Assembly_Operand operand = {};
		operand.constant_integer.integer = integer;
		operand.type = Op_Constant_Integer;

		return ASMA(operand);
	}

	Assembly_Operand* Builder::OpAdd(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Operand operand = {};
		operand.bin_op.operand1 = operand1;
		operand.bin_op.operand2 = operand2;

		operand.type = Op_Add;

		return ASMA(operand);
	}

	Assembly_Operand* Builder::OpSub(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Operand operand = {};
		operand.bin_op.operand1 = operand1;
		operand.bin_op.operand2 = operand2;

		operand.type = Op_Sub;

		return ASMA(operand);
	}

	Assembly_Operand* Builder::OpMul(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Operand operand = {};
		operand.bin_op.operand1 = operand1;
		operand.bin_op.operand2 = operand2;

		operand.type = Op_Mul;

		return ASMA(operand);
	}

	Assembly_Operand* Builder::OpDiv(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Operand operand = {};
		operand.bin_op.operand1 = operand1;
		operand.bin_op.operand2 = operand2;

		operand.type = Op_Div;

		return ASMA(operand);
	}

	Assembly_Operand* Builder::De_Reference(Assembly_Operand* operand1, TypeStorage* type /*= nullptr*/)
	{
		Assembly_Operand operand = {};
		operand.de_reference.operand = operand1;
		operand.de_reference.wordness = asm_none;

		if (type) {
			operand.de_reference.wordness = to_asm_size(TypeSystem::GetTypeSize(type));
		}

		operand.type = Op_De_Reference;

		return ASMA(operand);
	}

	std::unordered_map<X86_Register, std::string> Register_Names = {
		{X86_Register::RBP,"rbp"},
		{X86_Register::RSP,"rsp"},

		{X86_Register::AL,"al"},
		{X86_Register::BL,"bl"},

		{X86_Register::AX,"ax"},
		{X86_Register::BX,"bx"},

		{X86_Register::EAX,"eax"},
		{X86_Register::EBX,"ebx"},

		{X86_Register::RAX,"rax"},
		{X86_Register::RBX,"rbx"},
	};

	X86_BackEnd::X86_BackEnd(IRTranslationUnit* translation_unit, MetaData* metadata)
		: m_TranslationUnit(translation_unit), m_Metadata(metadata)
	{
		Init();
	}

	void X86_BackEnd::Init()
	{
	}

	std::string X86_BackEnd::Mangle_Name(const std::string& name, TypeStorage* type)
	{
		return fmt::format("{}_{}", name, (void*)type->Hash);
	}

	void X86_BackEnd::AssembleExternalFunction(const FunctionMetadata* function)
	{
		Assembly_External_Symbol external;
		external.Name = function->Symbol.Symbol;
		external.ExternalName = function->Symbol.Symbol;

		Externals.push_back(external);
	}

	void X86_BackEnd::AssembleExternals()
	{
		for (const auto& function_pair : m_Metadata->m_Functions) {

			const FunctionMetadata& function = function_pair.second;

			if (function.Foreign) {
				AssembleExternalFunction(&function);
			}
		}
	}

	void X86_BackEnd::Assemble()
	{
		AssembleExternals();

		for (auto i : m_TranslationUnit->Instructions) {
			if (i->GetType() == IRNodeType::File) {

				IRFile* ir_file = (IRFile*)i;

				for (auto tl_inst : ir_file->Instructions) {
					if (tl_inst->GetType() == IRNodeType::Function) {
						AssembleFunctionSymbol((IRFunction*)tl_inst);
					}
				}
			}
		}

		for (auto i : m_TranslationUnit->Instructions) {
			if (i->GetType() == IRNodeType::File) {

				IRFile* ir_file = (IRFile*)i;

				for (auto tl_inst : ir_file->Instructions) {
					AssembleInstruction(tl_inst);
				}
			}
			else {
				AssembleInstruction(i);
			}
		}

		Assembly_File assembly;
		assembly.externals = Externals;
		assembly.functions = Functions;

		FASM_Printer fasm_printer(&assembly);
		std::string fasm_output = fasm_printer.Print();

		if (!std::filesystem::exists(".build")) {
			std::filesystem::create_directory(".build");
		}

		{
			auto file_stream = std::ofstream(".build/fasm.s");
			file_stream << fasm_output;
		}

		GS_CORE_WARN("Running Fasm");

		system("fasm .build/fasm.s");

		GS_CORE_WARN("Running Linker On Fasm Output");

		system("clang .build/fasm.obj");
	}

	void X86_BackEnd::AssembleInstruction(IRInstruction* instruction)
	{
		switch (instruction->GetType()) {
		case IRNodeType::Function:
			AssembleFunction((IRFunction*)instruction);
			break;
		case IRNodeType::Register:
			AssembleRegister((IRRegister*)instruction);
			break;
		case IRNodeType::ConstValue:
			AssembleConstValue((IRCONSTValue*)instruction);
			break;
		case IRNodeType::Return:
			AssembleReturn((IRReturn*)instruction);
			break;
		default:
			GS_CORE_ASSERT(nullptr, "Un Implemented Instruction");
		}
	}

	void X86_BackEnd::AssembleFunctionSymbol(IRFunction* ir_function)
	{
		const auto metadata = m_Metadata->GetFunctionMetadata(ir_function->ID);
		const auto& name = metadata->Symbol.Symbol;

		Assembly_Function assembly;
		assembly.Name = name;

		Functions.push_back(assembly);

		m_Data.Functions[ir_function->ID] = &Functions[Functions.size() - 1];
	}

	void X86_BackEnd::AssembleFunction(IRFunction* ir_function)
	{
		const auto& metadata = m_Metadata->GetFunctionMetadata(ir_function->ID);

		Code.clear();

		m_Data.IR_RegisterValues.clear();
		m_Data.IR_RegisterTypes.clear();
		m_Data.Stack_Size = 0;

		Return_Storage_Location = nullptr;
		Return_Counter = 0;

		Assembly_Function* assembly = m_Data.Functions[ir_function->ID];

		Assembly_Operand* stack_size_constant = Builder::Constant_Integer(32);

		Code.push_back(Builder::Push(Builder::Register(RBP)));
		Code.push_back(Builder::Sub(Builder::Register(RSP), stack_size_constant));

		if (metadata->ReturnType != TypeSystem::GetVoid()) {
			Return_Storage_Location = Stack_Alloc(metadata->ReturnType);
		}

		for (auto instruction : ir_function->Instructions) {
			AssembleInstruction(instruction);
		}

		stack_size_constant->constant_integer.integer += m_Data.Stack_Size;
		stack_size_constant->constant_integer.integer = (i64)align_to(stack_size_constant->constant_integer.integer, 16);

		if (Return_Storage_Location) {
			auto return_location = GetReturnRegister(metadata->ReturnType);
			Code.push_back(Builder::Mov(return_location, Builder::De_Reference(Return_Storage_Location, metadata->ReturnType)));
		}

		Code.push_back(Builder::Add(Builder::Register(RSP), stack_size_constant));
		Code.push_back(Builder::Pop(Builder::Register(RBP)));
		Code.push_back(Builder::Ret());

		assembly->Code = Code;
	}

	void X86_BackEnd::AssembleRegister(IRRegister* ir_register)
	{
		CurrentRegister = ir_register->ID;

		AssembleInstruction(ir_register->Value);
	}

	void X86_BackEnd::AssembleReturn(IRReturn* ir_return)
	{
		Return_Counter++;
		Code.push_back(Builder::Mov(Builder::De_Reference(Return_Storage_Location, ir_return->Type), GetRegisterValue((IRRegisterValue*)ir_return->Value)));
	}

	void X86_BackEnd::AssembleConstValue(IRCONSTValue* ir_constant)
	{
		if (!(ir_constant->Type & FLAG_FLOATING_TYPE)) {
			SetRegisterValue(Builder::Constant_Integer(*(i64*)ir_constant->Data));
		}
		else {
			GS_CORE_ASSERT(nullptr);
		}
	}

	TypeStorage* X86_BackEnd::GetIRNodeType(IRInstruction* inst)
	{
		TypeStorage* type = nullptr;
		IRNodeType node_type = inst->GetType();
		return type;
	}

	Assembly_Operand* X86_BackEnd::Stack_Alloc(TypeStorage* type)
	{
		auto type_size = TypeSystem::GetTypeSize(type);
		m_Data.Stack_Size += type_size;
		return Builder::OpSub(Builder::Register(RBP), Builder::Constant_Integer(m_Data.Stack_Size));
	}

	Assembly_Operand* X86_BackEnd::GetReturnRegister(TypeStorage* type)
	{
		const std::unordered_map<u64, X86_Register> return_registers = {
			{1,AL},
			{2,AX},
			{4,EAX},
			{8,RAX},
		};

		auto type_size = TypeSystem::GetTypeSize(type);

		if (type_size > 8) {
			return Builder::Register(RAX);
		}
		else {
			return Builder::Register(return_registers.at(type_size));
		}
	}

	void X86_BackEnd::SetRegisterValue(Assembly_Operand* register_value)
	{
		m_Data.IR_RegisterValues[CurrentRegister] = register_value;
	}

	Assembly_Operand* X86_BackEnd::GetRegisterValue(IRRegisterValue* ir_register)
	{
		return m_Data.IR_RegisterValues.at(ir_register->RegisterID);
	}

	// 	//
	// 	// 
	// 	switch (node_type)
	// 	{
	// 	case IRNodeType::ConstValue:
	// 	{
	// 		IRCONSTValue* as_const_value = (IRCONSTValue*)inst;
	// 		type = TypeSystem::GetBasic(as_const_value->Type);
	// 	}
	// 	break;
	// 	case IRNodeType::DataValue:
	// 	{
	// 		type = TypeSystem::GetPtr(TypeSystem::GetBasic(IR_u8), 1);
	// 	}
	// 	break;
	// 	case IRNodeType::Alloca:
	// 	{
	// 		IRAlloca* as_alloca = (IRAlloca*)inst;
	// 		type = as_alloca->Type;
	// 	}
	// 	break;
	// 	case IRNodeType::Load:
	// 	{
	// 		IRLoad* as_load = (IRLoad*)inst;
	// 		type = as_load->Type;
	// 	}
	// 	break;
	// 	case IRNodeType::MemberAccess:
	// 	{
	// 		auto member_access = (IRMemberAccess*)inst;
	// 
	// 		const StructMetadata* struct_metadata = m_Metadata->GetStructMetadata(member_access->StructID);
	// 		GS_CORE_ASSERT(struct_metadata);
	// 		GS_CORE_ASSERT(member_access->MemberID < struct_metadata->Members.size());
	// 
	// 		const MemberMetadata& member = struct_metadata->Members[member_access->MemberID];
	// 		type = TypeSystem::GetPtr(member.Type, 1);
	// 	}
	// 	break;
	// 	case IRNodeType::ArrayAccess:
	// 	{
	// 		auto array_access = (IRArrayAccess*)inst;
	// 		type = TypeSystem::GetPtr(array_access->Type, 1);
	// 	}
	// 	break;
	// 	case IRNodeType::Store:
	// 	{
	// 		IRStore* as_store = (IRStore*)inst;
	// 		type = as_store->Type;
	// 	}
	// 	break;
	// 	case IRNodeType::Call:
	// 	{
	// 		IRFunctionCall* as_call = (IRFunctionCall*)inst;
	// 		type = m_Metadata->GetFunctionMetadata(as_call->FuncID)->ReturnType;
	// 	}
	// 	break;
	// 	case IRNodeType::ADD:
	// 	case IRNodeType::SUB:
	// 	case IRNodeType::MUL:
	// 	case IRNodeType::DIV:
	// 	{
	// 		IRBinOp* as_binop = (IRBinOp*)inst;
	// 		type = as_binop->Type;
	// 	}
	// 	break;
	// 	case IRNodeType::Argument:
	// 	{
	// 		GS_CORE_ASSERT(m_CurrentFunction);
	// 
	// 		IRArgumentAllocation* argument = (IRArgumentAllocation*)inst;
	// 		type = m_CurrentFunction->Arguments[argument->ArgumentIndex].Type;
	// 	}
	// 	break;
	// 	case IRNodeType::RegisterValue:
	// 	{
	// 		type = m_Data.IR_RegisterTypes.at(((IRRegisterValue*)inst)->RegisterID);
	// 	}
	// 	break;
	// 	case IRNodeType::GlobAddress:
	// 	{
	// 		type = m_Metadata->GetVariableMetadata(((IRGlobalAddress*)inst)->GlobID)->Tipe;
	// 	}
	// 	break;
	// 	case IRNodeType::NullPtr:
	// 	{
	// 		type = TypeSystem::GetPtr(TypeSystem::GetBasic(((IRNullPtr*)inst)->TypeID), 1);
	// 	}
	// 	break;
	// 	case IRNodeType::PointerCast:
	// 	{
	// 		type = ((IRPointerCast*)inst)->Type;
	// 	}
	// 	break;
	// 	case IRNodeType::IntTrunc:
	// 	case IRNodeType::Int2PtrCast:
	// 	case IRNodeType::Ptr2IntCast:
	// 	case IRNodeType::SExtCast:
	// 	case IRNodeType::ZExtCast:
	// 	case IRNodeType::FPTrunc:
	// 	case IRNodeType::FPExt:
	// 	{
	// 		type = ((IRIntTrunc*)inst)->Type;
	// 	}
	// 	break;
	// 	case IRNodeType::Int2FP:
	// 	{
	// 		type = ((IRInt2FP*)inst)->Type;
	// 	}
	// 	break;
	// 	case IRNodeType::GreaterThan:
	// 	case IRNodeType::LesserThan:
	// 	case IRNodeType::Equal:
	// 	case IRNodeType::NotEqual:
	// 	case IRNodeType::BitAnd:
	// 	case IRNodeType::BitOr:
	// 	case IRNodeType::And:
	// 	case IRNodeType::Or:
	// 	{
	// 		type = ((IRBinOp*)inst)->Type;
	// 	}
	// 	break;
	// 
	// 	case IRNodeType::TypeValue:
	// 	{
	// 		return TypeSystem::GetBasic(IR_type);
	// 	}
	// 	break;
	// 	case IRNodeType::TypeInfo:
	// 	case IRNodeType::TypeOf:
	// 	{
	// 		return TypeSystem::GetPtr(TypeSystem::GetBasic(IR_typeinfo), 1);
	// 	}
	// 	break;
	// 	case IRNodeType::Any:
	// 	{
	// 		return TypeSystem::GetBasic(IR_any);
	// 	}
	// 	case IRNodeType::AnyArray:
	// 	{
	// 		return TypeSystem::GetDynArray(TypeSystem::GetAny());
	// 	}
	// 	break;
	// 	case IRNodeType::FuncRef:
	// 	{
	// 		type = m_Metadata->GetFunctionMetadata(((IRFuncRef*)inst)->FunctionID)->Signature;
	// 	}
	// 	break;
	// 	case IRNodeType::CallFuncRef:
	// 	{
	// 		type = ((TSFunc*)((IRCallFuncRef*)inst)->Signature)->ReturnType;
	// 	}
	// 	break;
	// 
	// 	default:
	// 		GS_CORE_ASSERT(0);
	// 		break;
	// 	}

		//

	FASM_Printer::FASM_Printer(Assembly_File* assembly)
	{
		Assembly = assembly;
	}

	std::string FASM_Printer::Print()
	{
		std::stringstream stream;

		stream << "format MS64 COFF" << "\n" << "\n";
		stream << "public main" << "\n" << "\n";

		for (Assembly_External_Symbol external : Assembly->externals) {
			stream << "extrn '" << external.ExternalName << "' " << "as " << external.ExternalName << "\n";
		}

		PrintCode(stream);

		return stream.str();
	}

	void FASM_Printer::PrintOperand(const Assembly_Operand* operand, std::stringstream& stream)
	{
		static const std::unordered_map<Assembly_Size, std::string> wordness_map = {
			{asm_none, ""},
			{asm_byte,"byte "},
			{asm_word,"word "},
			{asm_dword,"dword "},
			{asm_qword,"qword "},
		};

		switch (operand->type)
		{
		case Op_Register:
			stream << Register_Names.at(operand->reg.Register);
			break;
		case Op_Constant_Integer:
			stream << operand->constant_integer.integer;
			break;

		case Op_Sub:
			PrintOperand(operand->bin_op.operand1, stream);
			stream << " - ";
			PrintOperand(operand->bin_op.operand2, stream);
			break;

		case Op_De_Reference:
			stream << wordness_map.at(operand->de_reference.wordness);
			stream << '[';
			PrintOperand(operand->de_reference.operand, stream);
			stream << ']';
			break;
		default:
			GS_CORE_ASSERT(nullptr, "Un Implemented Operand Instruction");
			break;
		}
	}

	void FASM_Printer::PrintInstruction(const Assembly_Instruction& instruction, std::stringstream& stream)
	{
		switch (instruction.OpCode)
		{
		case I_Ret:
			stream << "ret";
			break;
		case I_Push:
			stream << "push ";
			PrintOperand(instruction.Operand1, stream);
			break;
		case I_Pop:
			stream << "pop ";
			PrintOperand(instruction.Operand1, stream);
			break;
		case I_Add:
			stream << "add ";
			PrintOperand(instruction.Operand1, stream);
			stream << ", ";
			PrintOperand(instruction.Operand2, stream);
			break;
		case I_Sub:
			stream << "sub ";
			PrintOperand(instruction.Operand1, stream);
			stream << ", ";
			PrintOperand(instruction.Operand2, stream);
			break;

		case I_Mov:
			stream << "mov ";
			PrintOperand(instruction.Operand1, stream);
			stream << ", ";
			PrintOperand(instruction.Operand2, stream);
			break;

		default:
			GS_CORE_ASSERT(nullptr, "Un Implemented Assembly Instruction");
			break;
		}
	}

	void FASM_Printer::PrintCode(std::stringstream& stream)
	{
		stream << "\n";
		stream << "section '.code' code readable executable\n";
		stream << "\n";

		for (Assembly_Function& asm_function : Assembly->functions) {
			stream << asm_function.Name << ":" << "\n";

			for (auto& code : asm_function.Code) {
				stream << "\t";
				PrintInstruction(code, stream);
				stream << "\n";
			}
		}
	}
}