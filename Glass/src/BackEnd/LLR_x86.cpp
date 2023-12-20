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

	std::unordered_map<X86_Register, std::string> Register_Names = {
		{X86_Register::RBP,"rbp"},
		{X86_Register::RSP,"rsp"},

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
		default:
			GS_CORE_ASSERT(nullptr, "Un Implemented Instruction");
		}
	}

	void X86_BackEnd::AssembleFunctionSymbol(IRFunction* ir_function)
	{
		const auto& metadata = m_Metadata->GetFunctionMetadata(ir_function->ID);
		const auto& name = metadata->Symbol.Symbol;

		Assembly_Function assembly;
		assembly.Name = name;

		Functions.push_back(assembly);

		m_Data.Functions[ir_function->ID] = &Functions[Functions.size() - 1];
	}

	void X86_BackEnd::AssembleFunction(IRFunction* ir_function)
	{
		Code.clear();

		Assembly_Function* assembly = m_Data.Functions[ir_function->ID];

		Code.push_back(Builder::Push(Builder::Register(RBP)));
		Code.push_back(Builder::Sub(Builder::Register(RSP), Builder::Constant_Integer(32)));

		Code.push_back(Builder::Add(Builder::Register(RSP), Builder::Constant_Integer(32)));
		Code.push_back(Builder::Pop(Builder::Register(RBP)));

		Code.push_back(Builder::Ret());

		assembly->Code = Code;
	}

	TypeStorage* X86_BackEnd::GetIRNodeType(IRInstruction* inst)
	{
		TypeStorage* type = nullptr;
		IRNodeType node_type = inst->GetType();
		return type;
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
		switch (operand->type)
		{
		case Op_Register:
			stream << Register_Names.at(operand->reg.Register);
			break;
		case Op_Constant_Integer:
			stream << operand->constant_integer.integer;
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