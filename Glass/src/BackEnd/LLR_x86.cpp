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

	Assembly_Instruction Builder::Build_Inst(Assembly_Op_Code op_code, Assembly_Operand* op1 /*= nullptr*/, Assembly_Operand* op2 /*= nullptr*/)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = op_code;
		instruction.Operand1 = op1;
		instruction.Operand2 = op2;

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

	Assembly_Instruction Builder::AddSS(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_AddSS;
		instruction.Operand1 = operand1;
		instruction.Operand2 = operand2;

		return instruction;
	}

	Assembly_Instruction Builder::AddSD(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_AddSD;
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

	Assembly_Instruction Builder::SubSS(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_SubSS;
		instruction.Operand1 = operand1;
		instruction.Operand2 = operand2;

		return instruction;
	}

	Assembly_Instruction Builder::SubSD(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_SubSD;
		instruction.Operand1 = operand1;
		instruction.Operand2 = operand2;

		return instruction;
	}

	Assembly_Instruction Builder::Mul(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_IMul;
		instruction.Operand1 = operand1;
		instruction.Operand2 = operand2;

		return instruction;
	}

	Assembly_Instruction Builder::MulSS(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_MulSS;
		instruction.Operand1 = operand1;
		instruction.Operand2 = operand2;

		return instruction;
	}

	Assembly_Instruction Builder::MulSD(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_MulSD;
		instruction.Operand1 = operand1;
		instruction.Operand2 = operand2;

		return instruction;
	}

	Assembly_Instruction Builder::IDiv(Assembly_Operand* operand1)
	{
		return Builder::Build_Inst(I_IDiv, operand1);
	}

	Assembly_Instruction Builder::Div(Assembly_Operand* operand1)
	{
		return Builder::Build_Inst(I_Div, operand1);
	}

	Assembly_Instruction Builder::DivSS(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		return Builder::Build_Inst(I_DivSS, operand1, operand2);
	}

	Assembly_Instruction Builder::DivSD(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		return Builder::Build_Inst(I_DivSD, operand1, operand2);
	}

	Assembly_Instruction Builder::Call(Assembly_Operand* operand1)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_Call;
		instruction.Operand1 = operand1;

		return instruction;
	}

	Assembly_Instruction Builder::Lea(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_Lea;
		instruction.Operand1 = operand1;
		instruction.Operand2 = operand2;
		return instruction;
	}

	Assembly_Instruction Builder::SS2SD(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_CvtSS2SD;
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

	Assembly_Instruction Builder::MovD(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_MovD;
		instruction.Operand1 = operand1;
		instruction.Operand2 = operand2;

		return instruction;
	}

	Assembly_Instruction Builder::MovQ(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_MovQ;
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

	Assembly_Operand* Builder::Symbol(const std::string& symbol)
	{
		Assembly_Operand operand = {};

		operand.symbol.symbol = ASMA(symbol)->data();

		operand.type = Op_Symbol;

		return ASMA(operand);
	}

	std::unordered_map<X86_Register, std::string> Register_Names = {
		{X86_Register::RBP,"rbp"},
		{X86_Register::RSP,"rsp"},

		{X86_Register::AL,"al"},
		{X86_Register::BL,"bl"},
		{X86_Register::CL,"cl"},
		{X86_Register::DL,"dl"},
		{X86_Register::R8b,"r8b"},
		{X86_Register::R9b,"r9b"},

		{X86_Register::AX,"ax"},
		{X86_Register::BX,"bx"},
		{X86_Register::CX,"cx"},
		{X86_Register::DX,"dx"},
		{X86_Register::R8w,"r8w"},
		{X86_Register::R9w,"r9w"},

		{X86_Register::EAX,"eax"},
		{X86_Register::EBX,"ebx"},
		{X86_Register::ECX,"ecx"},
		{X86_Register::EDX,"edx"},
		{X86_Register::R8d,"r8d"},
		{X86_Register::R9d,"r9d"},
		{X86_Register::R10d,"r10d"},
		{X86_Register::R11d,"r11d"},
		{X86_Register::R12d,"r12d"},
		{X86_Register::R13d,"r13d"},
		{X86_Register::R14d,"r14d"},
		{X86_Register::R15d,"r15d"},

		{X86_Register::RAX,"rax"},
		{X86_Register::RBX,"rbx"},
		{X86_Register::RCX,"rcx"},
		{X86_Register::RDX,"rdx"},
		{X86_Register::R8,"r8"},
		{X86_Register::R9,"r9"},
		{X86_Register::R10,"r10"},
		{X86_Register::R11,"r11"},
		{X86_Register::R12,"r12"},
		{X86_Register::R13,"r13"},
		{X86_Register::R14,"r14"},
		{X86_Register::R15,"r15"},

		{X86_Register::XMM0,"xmm0"},
		{X86_Register::XMM1,"xmm1"},
		{X86_Register::XMM2,"xmm2"},
		{X86_Register::XMM3,"xmm3"},
		{X86_Register::XMM4,"xmm4"},
		{X86_Register::XMM5,"xmm5"},
		{X86_Register::XMM6,"xmm6"},
		{X86_Register::XMM7,"xmm7"},

	};

	X86_BackEnd::X86_BackEnd(IRTranslationUnit* translation_unit, MetaData* metadata)
		: m_TranslationUnit(translation_unit), m_Metadata(metadata)
	{
		Init();
	}

	void X86_BackEnd::Init()
	{
		Register_Allocator_Data.allocated = {
			{F_A, false},
			{F_B, false},
			{F_C, false},
			{F_D, false},
			{F_R8, false},
			{F_R9, false},
			{F_R10, false},
			{F_R11, false},
			{F_R12, false},
			{F_R13, false},
			{F_R14, false},
			{F_R15, false},
		};

		Register_Allocator_Data.allocated_floating = {
			{F_X0, false},
			{F_X1, false},
			{F_X2, false},
			{F_X3, false},
			{F_X4, false},
			{F_X5, false},
			{F_X6, false},
			{F_X7, false},
		};
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

		std::chrono::steady_clock::time_point Start;
		std::chrono::steady_clock::time_point End;

		Start = std::chrono::high_resolution_clock::now();

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
		assembly.floats = Floats;
		assembly.strings = Strings;

		for (auto func : Functions) {
			assembly.functions.push_back(*func);
		}

		End = std::chrono::high_resolution_clock::now();

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

		std::chrono::steady_clock::time_point Fasm_Start = std::chrono::high_resolution_clock::now();
		system("fasm .build/fasm.s");
		std::chrono::steady_clock::time_point Fasm_End = std::chrono::high_resolution_clock::now();

		GS_CORE_WARN("Running Linker On Fasm Output");

		std::chrono::steady_clock::time_point Linker_Start = std::chrono::high_resolution_clock::now();
		system("clang ./.build/fasm.obj -O0");
		std::chrono::steady_clock::time_point Linker_End = std::chrono::high_resolution_clock::now();

		GS_CORE_WARN("Assembly Generation Took: {} milli s", std::chrono::duration_cast<std::chrono::milliseconds>(End - Start).count());
		GS_CORE_WARN("FASM Took: {} mill s", std::chrono::duration_cast<std::chrono::milliseconds>(Fasm_End - Fasm_Start).count());
		GS_CORE_WARN("Linker Took: {} mill s", std::chrono::duration_cast<std::chrono::milliseconds>(Linker_End - Linker_Start).count());
	}

	void X86_BackEnd::AssembleInstruction(IRInstruction* instruction)
	{
		switch (instruction->GetType()) {
		case IRNodeType::Function:
			AssembleFunction((IRFunction*)instruction);
			break;
		case IRNodeType::Call:
			AssembleFunctionCall((IRFunctionCall*)instruction);
			break;
		case IRNodeType::Register:
			AssembleRegister((IRRegister*)instruction);
			break;
		case IRNodeType::Argument:
			AssembleArgument((IRArgumentAllocation*)instruction);
			break;
		case IRNodeType::Alloca:
			AssembleAlloca((IRAlloca*)instruction);
			break;
		case IRNodeType::Store:
			AssembleStore((IRStore*)instruction);
			break;
		case IRNodeType::Load:
			AssembleLoad((IRLoad*)instruction);
			break;
		case IRNodeType::ConstValue:
			AssembleConstValue((IRCONSTValue*)instruction);
			break;
		case IRNodeType::Data:
			AssembleData((IRData*)instruction);
			break;
		case IRNodeType::DataValue:
			AssembleDataValue((IRDataValue*)instruction);
			break;
		case IRNodeType::ADD:
			AssembleAdd((IRADD*)instruction);
			break;
		case IRNodeType::SUB:
			AssembleSub((IRSUB*)instruction);
			break;
		case IRNodeType::MUL:
			AssembleMul((IRMUL*)instruction);
			break;
		case IRNodeType::DIV:
			AssembleDiv((IRDIV*)instruction);
			break;
		case IRNodeType::Return:
			AssembleReturn((IRReturn*)instruction);
			break;
		case IRNodeType::MemberAccess:
			AssembleMemberAccess((IRMemberAccess*)instruction);
			break;
		case IRNodeType::RegisterValue:
		{
			IRRegisterValue* ir_register_value = (IRRegisterValue*)instruction;

			auto register_value = GetRegisterValue(ir_register_value);
			auto register_value_type = GetRegisterValueType(ir_register_value);

			if (register_value->type != Op_Register) {
				auto result_location = Allocate_Register(TypeSystem::GetVoidPtr(), CurrentRegister);

				if (register_value_type != Register_Value_Type::Pointer_Address && register_value_type != Register_Value_Type::Stack_Address) {
					GS_CORE_ASSERT(nullptr);
				}

				Code.push_back(Builder::Lea(result_location, Builder::De_Reference(register_value)));

				SetRegisterValue(result_location, CurrentRegister, Register_Value_Type::Pointer_Address);
			}
		}
		break;
		default:
			GS_CORE_ASSERT(nullptr, "Un Implemented Instruction");
		}
	}

	void X86_BackEnd::AssembleFunctionSymbol(IRFunction* ir_function)
	{
		const auto metadata = m_Metadata->GetFunctionMetadata(ir_function->ID);
		const auto& name = metadata->Symbol.Symbol;

		Assembly_Function* assembly = ASMA(Assembly_Function());

		assembly->Name = name;

		if (name != "main") {
			assembly->Name = Mangle_Name(name, metadata->Signature);
		}

		Functions.push_back(assembly);
		m_Data.Functions[ir_function->ID] = assembly;
	}

	const std::map<std::pair<u64, u64>, X86_Register> argument_register_map = {
		{ {8,0}, RCX},
		{ {8,1}, RDX},
		{ {8,2}, R8},
		{ {8,3}, R9},

		{ {4,0}, ECX},
		{ {4,1}, EDX},
		{ {4,2}, R8d},
		{ {4,3}, R9d},

		{ {2,0}, CX},
		{ {2,1}, DX},
		{ {2,2}, R8w},
		{ {2,3}, R9w},

		{ {1,0}, CL},
		{ {1,1}, DL},
		{ {1,2}, R8b},
		{ {1,3}, R9b},
	};

	const std::map<u64, X86_Register> argument_float_register_map = {
		{ 0, XMM0},
		{ 1, XMM1},
		{ 2, XMM2},
		{ 3, XMM3},
	};

	void X86_BackEnd::AssembleFunction(IRFunction* ir_function)
	{
		const auto& metadata = m_Metadata->GetFunctionMetadata(ir_function->ID);

		Code.clear();

		m_Data.IR_RegisterValues.clear();
		m_Data.IR_RegisterTypes.clear();
		m_Data.IR_RegisterValueTypes.clear();
		m_Data.IR_RegisterLifetimes.clear();
		m_Data.Stack_Size = 0;
		m_Data.Call_Stack_Pointer = 0;
		m_Data.Call_Stack_Size = 0;

		Return_Storage_Location = nullptr;
		Return_Counter = 0;
		Return_Encountered = false;

		Register_Allocator_Data.allocations.clear();
		Register_Allocator_Data.family_to_allocation.clear();

		for (auto& [family, allocated] : Register_Allocator_Data.allocated) {
			allocated = false;
		}

		for (auto& [family, allocated] : Register_Allocator_Data.allocated_floating) {
			allocated = false;
		}

		Assembly_Function* assembly = m_Data.Functions[ir_function->ID];

		Assembly_Operand* stack_size_constant = Builder::Constant_Integer(32);

		Code.push_back(Builder::Push(Builder::Register(RBP)));
		Code.push_back(Builder::Sub(Builder::Register(RSP), stack_size_constant));

		Code.push_back(Builder::Lea(Builder::Register(RBP), Builder::De_Reference(Builder::OpAdd(Builder::Register(RSP), stack_size_constant))));

		if (metadata->ReturnType != TypeSystem::GetVoid()) {
			Return_Storage_Location = Stack_Alloc(metadata->ReturnType);
		}

		u64 i = 0;

		for (const ArgumentMetadata& argument : metadata->Arguments) {
			auto type_size = TypeSystem::GetTypeSize(argument.Type);

			CurrentRegister = argument.AllocationLocation->RegisterID;

			if (type_size <= 8) {
				if (i < 4)
				{
					X86_Register needed_register;

					if (!TypeSystem::IsFlt(argument.Type)) {
						needed_register = argument_register_map.at({ type_size, i });
					}
					else {
						needed_register = argument_float_register_map.at(i);
					}

					SetRegisterValue(Allocate_Register(argument.Type, CurrentRegister, needed_register), Register_Value_Type::Register_Value);
				}
				else {
					SetRegisterValue(Builder::De_Reference(Builder::OpAdd(Builder::Register(RBP), Builder::Constant_Integer(40 + (i - 3) * 8)), argument.Type), Register_Value_Type::Memory_Value);
				}
			}
			else {
				GS_CORE_ASSERT(nullptr);
			}
			i++;
		}

		for (auto instruction : ir_function->Instructions) {

			if (Return_Encountered) {
				Return_Encountered = false;
				break;
			}

			AssembleInstruction(instruction);
		}

		stack_size_constant->constant_integer.integer += m_Data.Stack_Size;
		stack_size_constant->constant_integer.integer += m_Data.Call_Stack_Size + 8;
		stack_size_constant->constant_integer.integer = (i64)align_to(stack_size_constant->constant_integer.integer, 16);

		if (Return_Storage_Location) {
			auto return_location = GetReturnRegister(metadata->ReturnType);
			Code.push_back(MoveBasedOnType(metadata->ReturnType, return_location, Builder::De_Reference(Return_Storage_Location, metadata->ReturnType)));
		}

		Code.push_back(Builder::Add(Builder::Register(RSP), stack_size_constant));
		Code.push_back(Builder::Pop(Builder::Register(RBP)));
		Code.push_back(Builder::Ret());

		assembly->Code = Code;
	}

	void X86_BackEnd::AssembleFunctionCall(IRFunctionCall* ir_function)
	{
		const FunctionMetadata* metadata = m_Metadata->GetFunctionMetadata(ir_function->FuncID);
		std::string name = metadata->Symbol.Symbol;

		if (!metadata->Foreign) {
			name = Mangle_Name(name, metadata->Signature);
		}

		static std::map<u64, X86_Register> return_register_map = {
			{1, AL},
			{2, AX},
			{4, EAX},
			{8, RAX},
		};

		std::vector<u64> argument_allocations;
		std::vector <std::pair<Assembly_Operand*, Assembly_Operand*>> argument_allocations_registers;

		for (size_t i = 0; i < ir_function->Arguments.size(); i++)
		{
			auto call_argument = (IRRegisterValue*)ir_function->Arguments[i];

			TypeStorage* argument_type = ir_function->ArgumentTypes[i];

			auto type_size = TypeSystem::GetTypeSize(argument_type);

			if (type_size <= 8) {

				X86_Register needed_register;

				if (i < 4)
				{
					if (!TypeSystem::IsFlt(argument_type)) {
						needed_register = argument_register_map.at({ type_size,i });
					}
					else {
						needed_register = argument_float_register_map.at(i);
					}

					auto temp_register_id = CreateTempRegister(nullptr);
					auto argument_phys_register = Allocate_Register(argument_type, temp_register_id, needed_register);

					argument_allocations_registers.push_back({ argument_phys_register , nullptr });

					SetRegisterValue(argument_phys_register, temp_register_id);

					auto call_argument_value = GetRegisterValue(call_argument);

					Code.push_back(MoveBasedOnType(argument_type, argument_phys_register, call_argument_value));

					if (metadata->Variadic) {
						if (TypeSystem::IsFlt(argument_type)) {

							auto argument_type_equ_int = TypeSystem::GetI64();
							auto argument_type_equ_int_type_size = TypeSystem::GetTypeSize(argument_type_equ_int);

							needed_register = argument_register_map.at({ argument_type_equ_int_type_size,i });;

							auto integer_register_id = CreateTempRegister(nullptr);
							auto integer_phys_register = Allocate_Register(argument_type_equ_int, integer_register_id, needed_register);
							SetRegisterValue(integer_phys_register, integer_register_id);

							argument_allocations.push_back(integer_register_id);
							argument_allocations_registers[i].second = integer_phys_register;
						}
					}

					argument_allocations.push_back(temp_register_id);
				}
			}
		}

		for (size_t i = 0; i < ir_function->Arguments.size(); i++)
		{
			auto call_argument = (IRRegisterValue*)ir_function->Arguments[i];

			TypeStorage* argument_type = ir_function->ArgumentTypes[i];

			auto type_size = TypeSystem::GetTypeSize(argument_type);

			if (type_size <= 8) {

				X86_Register needed_register;

				if (i < 4)
				{
					if (!TypeSystem::IsFlt(argument_type)) {
						needed_register = argument_register_map.at({ type_size,i });
					}
					else {
						needed_register = argument_float_register_map.at(i);
					}

					GS_CORE_ASSERT(argument_allocations_registers[i].first);
					auto argument_phys_register = argument_allocations_registers[i].first;

					auto call_argument_value = GetRegisterValue(call_argument);

					Code.push_back(MoveBasedOnType(argument_type, argument_phys_register, call_argument_value));

					if (metadata->Variadic) {
						if (TypeSystem::IsFlt(argument_type)) {

							if (type_size != 8) {
								Code.push_back(Builder::SS2SD(argument_phys_register, argument_phys_register));
							}

							GS_CORE_ASSERT(argument_allocations_registers[i].second);

							auto integer_phys_register = argument_allocations_registers[i].second;

							Code.push_back(Builder::MovQ(integer_phys_register, argument_phys_register));
						}
					}
				}
				else {
					auto call_argument_value = GetRegisterValue(call_argument);

					if (GetRegisterValueType(call_argument) != Register_Value_Type::Register_Value) {

						auto tmp_move_register_id = CreateTempRegister(nullptr);
						auto tmp_move_register = Allocate_Register(argument_type, tmp_move_register_id);
						SetRegisterValue(tmp_move_register, tmp_move_register_id, Register_Value_Type::Register_Value);
						UseRegisterValue(tmp_move_register_id);

						Code.push_back(MoveBasedOnType(argument_type, tmp_move_register, call_argument_value));

						call_argument_value = tmp_move_register;
					}

					if (metadata->Variadic) {
						if (TypeSystem::IsFlt(argument_type)) {

							if (type_size != 8) {
								Code.push_back(Builder::SS2SD(call_argument_value, call_argument_value));
								argument_type = TypeSystem::GetBasic(IR_f64);
							}
						}
					}

					auto argument_stack_location = Builder::De_Reference(Alloc_Call_StackTop(TypeSystem::GetU64()), argument_type);

					Code.push_back(MoveBasedOnType(argument_type, argument_stack_location, call_argument_value));
				}

				UseRegisterValue(call_argument);
			}
			else {
				GS_CORE_ASSERT(nullptr);
			}
		}

		Assembly_Operand* return_location = nullptr;

		if (metadata->ReturnType != TypeSystem::GetVoid()) {
			auto return_type_size = TypeSystem::GetTypeSize(metadata->ReturnType);

			if (!TypeSystem::IsFlt(metadata->ReturnType)) {
				return_location = Allocate_Register(metadata->ReturnType, CurrentRegister, return_register_map.at(return_type_size));
			}
			else {
				return_location = Allocate_Register(metadata->ReturnType, CurrentRegister, XMM0);
			}
		}

		Code.push_back(Builder::Call(Builder::Symbol(name)));

		SetRegisterValue(return_location, Register_Value_Type::Register_Value);

		if (!TypeSystem::IsPointer(metadata->ReturnType)) {
			if (m_Metadata->GetStructIDFromType(metadata->ReturnType->BaseID) != -1) {

				auto new_return_location = Stack_Alloc(metadata->ReturnType);

				Code.push_back(Builder::Mov(Builder::De_Reference(new_return_location, metadata->ReturnType), return_location));

				return_location = new_return_location;
			}

			SetRegisterValue(return_location, Register_Value_Type::Stack_Address);
		}

		for (auto allocation : argument_allocations) {
			UseRegisterValue(allocation);
		}

		m_Data.Call_Stack_Pointer = 0;
	}

	void X86_BackEnd::AssembleRegister(IRRegister* ir_register)
	{
		CurrentRegister = ir_register->ID;
		AssembleInstruction(ir_register->Value);
	}

	void X86_BackEnd::AssembleArgument(IRArgumentAllocation* ir_argument)
	{
		auto argument_input_location = GetRegisterValue(CurrentRegister);
		UseRegisterValue(CurrentRegister);

		auto argument_storage_location = Stack_Alloc(ir_argument->AllocationType);

		if (argument_input_location->type != Op_Register) {

			auto tmp_move_register_id = CreateTempRegister(nullptr);
			auto tmp_move_register = Allocate_Register(ir_argument->AllocationType, tmp_move_register_id);
			SetRegisterValue(tmp_move_register, tmp_move_register_id, Register_Value_Type::Register_Value);
			UseRegisterValue(tmp_move_register_id);

			Code.push_back(MoveBasedOnType(ir_argument->AllocationType, tmp_move_register, argument_input_location));

			argument_input_location = tmp_move_register;
		}

		Code.push_back(MoveBasedOnType(ir_argument->AllocationType, Builder::De_Reference(argument_storage_location, ir_argument->AllocationType), argument_input_location));

		SetRegisterValue(argument_storage_location, Register_Value_Type::Stack_Address);

		m_Data.IR_RegisterLifetimes.at(CurrentRegister) = 1;
	}

	void X86_BackEnd::AssembleAlloca(IRAlloca* ir_alloca)
	{
		auto register_value = Stack_Alloc(ir_alloca->Type);
		SetRegisterValue(register_value, Register_Value_Type::Stack_Address);
	}

	void X86_BackEnd::AssembleMemberAccess(IRMemberAccess* ir_member_access)
	{
		const StructMetadata* struct_metadata = m_Metadata->GetStructMetadata(ir_member_access->StructID);

		const MemberMetadata& member = struct_metadata->Members[ir_member_access->MemberID];

		auto offset = member.Offset;

		auto object_value = GetRegisterValue(ir_member_access->ObjectRegister);

		auto register_value_type = GetRegisterValueType(ir_member_access->ObjectRegister);

		if (ir_member_access->ReferenceAccess || register_value_type == Register_Value_Type::Memory_Value) {

			GS_CORE_ASSERT(object_value->type != Op_Register);

			UseRegisterValue(ir_member_access->ObjectRegister);
			auto new_object_value = Allocate_Register(TypeSystem::GetVoidPtr(), CurrentRegister);

			if (register_value_type != Register_Value_Type::Memory_Value && register_value_type != Register_Value_Type::Register_Value) {
				object_value = Builder::De_Reference(object_value, TypeSystem::GetVoidPtr());
			}

			register_value_type = Register_Value_Type::Register_Value;

			Code.push_back(MoveBasedOnType(TypeSystem::GetVoidPtr(), new_object_value, object_value));

			object_value = new_object_value;
		}

		if (register_value_type == Register_Value_Type::Stack_Address) {

			GS_CORE_ASSERT(object_value->bin_op.operand1->reg.Register == RBP);
			GS_CORE_ASSERT(object_value->bin_op.operand2->type == Op_Constant_Integer);

			SetRegisterValue(Builder::OpSub(Builder::Register(RBP), Builder::Constant_Integer(object_value->bin_op.operand2->constant_integer.integer - offset)), CurrentRegister, Register_Value_Type::Stack_Address);
			UseRegisterValue(ir_member_access->ObjectRegister);
		}
		else if (register_value_type == Register_Value_Type::Pointer_Address) {
			GS_CORE_ASSERT(object_value->bin_op.operand1->reg.Register != RBP);
			GS_CORE_ASSERT(object_value->bin_op.operand2->type == Op_Constant_Integer);

			UseRegisterValue(ir_member_access->ObjectRegister);
			Allocate_Register(TypeSystem::GetVoidPtr(), CurrentRegister, object_value->bin_op.operand1->reg.Register);

			SetRegisterValue(Builder::OpAdd(Builder::Register(object_value->bin_op.operand1->reg.Register), Builder::Constant_Integer(object_value->bin_op.operand2->constant_integer.integer + offset)), CurrentRegister, Register_Value_Type::Stack_Address);
		}
		else if (register_value_type == Register_Value_Type::Register_Value) {

			if (!ir_member_access->ReferenceAccess) {
				UseRegisterValue(ir_member_access->ObjectRegister);
				Allocate_Register(TypeSystem::GetVoidPtr(), CurrentRegister, object_value->reg.Register);
			}

			SetRegisterValue(Builder::OpAdd(Builder::Register(object_value->reg.Register), Builder::Constant_Integer(offset)), CurrentRegister, Register_Value_Type::Pointer_Address);
		}
		else {
			GS_CORE_ASSERT(nullptr);
		}
	}

	void X86_BackEnd::AssembleStore(IRStore* ir_store)
	{
		auto type_size = TypeSystem::GetTypeSize(ir_store->Type);

		auto pointer_register_value = GetRegisterValue(ir_store->AddressRegister);

		auto data_register_value = GetRegisterValue((IRRegisterValue*)ir_store->Data);

		if (data_register_value->type != Op_Register) {

			auto temp_register_id = CreateTempRegister(nullptr);

			auto temp_phys_register = Allocate_Register(ir_store->Type, temp_register_id);

			SetRegisterValue(temp_phys_register, temp_register_id);

			Code.push_back(MoveBasedOnType(ir_store->Type, temp_phys_register, data_register_value));

			UseRegisterValue(temp_register_id);

			data_register_value = temp_phys_register;
		}

		if (type_size <= 8) {
			auto move = MoveBasedOnType(ir_store->Type, Builder::De_Reference(pointer_register_value, ir_store->Type), data_register_value);
			Code.push_back(move);
		}
		else {
			GS_CORE_ASSERT(nullptr);
		}

		UseRegisterValue(ir_store->AddressRegister);
		UseRegisterValue((IRRegisterValue*)ir_store->Data);
	}

	void X86_BackEnd::AssembleLoad(IRLoad* ir_load)
	{
		auto type_size = TypeSystem::GetTypeSize(ir_load->Type);

		auto pointer_register_value = GetRegisterValue(ir_load->AddressRegister);

		if (type_size <= 8) {
			auto loaded_data_register = Allocate_Register(ir_load->Type, CurrentRegister);
			Code.push_back(MoveBasedOnType(ir_load->Type, loaded_data_register, Builder::De_Reference(pointer_register_value, ir_load->Type)));
			SetRegisterValue(loaded_data_register, Register_Value_Type::Register_Value);
		}
		else {
			GS_CORE_ASSERT(nullptr);
		}

		UseRegisterValue(ir_load->AddressRegister);

	}

	void X86_BackEnd::AssembleAdd(IRADD* ir_add)
	{
		auto result_location = Allocate_Register(ir_add->Type, CurrentRegister);

		auto a_value = GetRegisterValue(ir_add->RegisterA);
		auto b_value = GetRegisterValue(ir_add->RegisterB);

		if (!Are_Equal(result_location, a_value)) {

			Code.push_back(MoveBasedOnType(ir_add->Type, result_location, a_value));
		}

		SetRegisterValue(result_location, Register_Value_Type::Register_Value);

		if (TypeSystem::IsFlt(ir_add->Type)) {

			auto type_size = TypeSystem::GetTypeSize(ir_add->Type);

			if (type_size == 4) {
				Code.push_back(Builder::AddSS(result_location, b_value));
			}
			else if (type_size == 8) {
				Code.push_back(Builder::AddSD(result_location, b_value));
			}
			else {
				GS_CORE_ASSERT(nullptr);
			}
		}
		else {
			Code.push_back(Builder::Add(result_location, b_value));
		}

		UseRegisterValue(ir_add->RegisterA);
		UseRegisterValue(ir_add->RegisterB);

	}

	void X86_BackEnd::AssembleSub(IRSUB* ir_sub)
	{
		auto result_location = Allocate_Register(ir_sub->Type, CurrentRegister);

		auto a_value = GetRegisterValue(ir_sub->RegisterA);
		auto b_value = GetRegisterValue(ir_sub->RegisterB);

		if (!Are_Equal(result_location, a_value)) {
			Code.push_back(MoveBasedOnType(ir_sub->Type, result_location, a_value));
		}

		SetRegisterValue(result_location, Register_Value_Type::Register_Value);
		if (TypeSystem::IsFlt(ir_sub->Type)) {

			auto type_size = TypeSystem::GetTypeSize(ir_sub->Type);

			if (type_size == 4) {
				Code.push_back(Builder::SubSS(result_location, b_value));
			}
			else if (type_size == 8) {
				Code.push_back(Builder::SubSD(result_location, b_value));
			}
			else {
				GS_CORE_ASSERT(nullptr);
			}
		}
		else {
			Code.push_back(Builder::Sub(result_location, b_value));
		}

		UseRegisterValue(ir_sub->RegisterA);
		UseRegisterValue(ir_sub->RegisterB);
	}

	void X86_BackEnd::AssembleMul(IRMUL* ir_mul)
	{
		auto result_location = Allocate_Register(ir_mul->Type, CurrentRegister);

		auto a_value = GetRegisterValue(ir_mul->RegisterA);
		auto b_value = GetRegisterValue(ir_mul->RegisterB);

		if (!Are_Equal(result_location, a_value)) {

			Code.push_back(MoveBasedOnType(ir_mul->Type, result_location, a_value));
		}

		SetRegisterValue(result_location, Register_Value_Type::Register_Value);

		if (TypeSystem::IsFlt(ir_mul->Type)) {

			auto type_size = TypeSystem::GetTypeSize(ir_mul->Type);

			if (type_size == 4) {
				Code.push_back(Builder::MulSS(result_location, b_value));
			}
			else if (type_size == 8) {
				Code.push_back(Builder::MulSD(result_location, b_value));
			}
			else {
				GS_CORE_ASSERT(nullptr);
			}
		}
		else {
			Code.push_back(Builder::Mul(result_location, b_value));
		}

		UseRegisterValue(ir_mul->RegisterA);
		UseRegisterValue(ir_mul->RegisterB);
	}

	struct Division_Inst_Registers {
		X86_Register first_half;
		X86_Register second_half;
		X86_Register result;
		X86_Register remainder;
	};

	const std::map<u64, Division_Inst_Registers> divide_register_table = {
		{2, {AX, DX, AX, DX}},
		{4, {EAX, EDX, EAX, EDX}},
		{8, {RAX, RDX, RAX, RDX}},
	};

	void X86_BackEnd::AssembleDiv(IRDIV* ir_div)
	{
		auto type_size = TypeSystem::GetTypeSize(ir_div->Type);
		auto type_flags = TypeSystem::GetTypeFlags(ir_div->Type);

		if (TypeSystem::IsFlt(ir_div->Type)) {

			auto input_result_location = Allocate_Register(ir_div->Type, CurrentRegister);

			auto a_value = GetRegisterValue(ir_div->RegisterA);
			auto b_value = GetRegisterValue(ir_div->RegisterB);

			// 			if (GetRegisterLiveness(ir_div->RegisterA) == Register_Liveness::Address_To_Value) {
			// 				a_value = Builder::De_Reference(a_value, ir_div->Type);
			// 			}
			// 
			// 			if (GetRegisterLiveness(ir_div->RegisterB) == Register_Liveness::Address_To_Value) {
			// 				b_value = Builder::De_Reference(b_value, ir_div->Type);
			// 			}

			Code.push_back(MoveBasedOnType(ir_div->Type, input_result_location, a_value));

			if (b_value->type == Op_Constant_Integer)
			{
				auto divisor_reg_id = CreateTempRegister(nullptr);

				auto divisor_reg_value = Allocate_Register(ir_div->Type, divisor_reg_id);
				SetRegisterValue(divisor_reg_value, divisor_reg_id, Register_Value_Type::Register_Value);

				Code.push_back(MoveBasedOnType(ir_div->Type, divisor_reg_value, b_value));

				UseRegisterValue(divisor_reg_id);
				b_value = divisor_reg_value;
			}

			if (type_size == 4) {
				Code.push_back(Builder::DivSS(input_result_location, b_value));
			}
			else if (type_size == 8) {
				Code.push_back(Builder::DivSD(input_result_location, b_value));
			}
			else {
				GS_CORE_ASSERT(nullptr);
			}

			SetRegisterValue(input_result_location, Register_Value_Type::Register_Value);
		}
		else {

			bool is_unsigned = (type_flags & FLAG_UNSIGNED_TYPE);

			const auto& register_table = divide_register_table.at(type_size);

			auto remainder_register = Builder::Register(register_table.remainder);
			auto input_result_location = Allocate_Register(ir_div->Type, CurrentRegister, register_table.result);

			auto rem_reg_id = CreateTempRegister(remainder_register);
			remainder_register = Allocate_Register(ir_div->Type, rem_reg_id, register_table.remainder);

			if (is_unsigned) {
				Code.push_back(Builder::Mov(remainder_register, Builder::Constant_Integer(0)));
			}

			auto a_value = GetRegisterValue(ir_div->RegisterA);
			auto b_value = GetRegisterValue(ir_div->RegisterB);

			if (b_value->type == Op_Constant_Integer)
			{
				auto divisor_reg_id = CreateTempRegister(nullptr);

				auto divisor_reg_value = Allocate_Register(ir_div->Type, divisor_reg_id);
				SetRegisterValue(divisor_reg_value, divisor_reg_id, Register_Value_Type::Register_Value);

				Code.push_back(Builder::Mov(divisor_reg_value, b_value));

				UseRegisterValue(divisor_reg_id);
				b_value = divisor_reg_value;
			}

			// 			if (GetRegisterLiveness(ir_div->RegisterA) == Register_Liveness::Address_To_Value) {
			// 				a_value = Builder::De_Reference(a_value, ir_div->Type);
			// 			}
			// 
			// 			if (GetRegisterLiveness(ir_div->RegisterB) == Register_Liveness::Address_To_Value) {
			// 				b_value = Builder::De_Reference(b_value, ir_div->Type);
			// 			}

			Code.push_back(Builder::Mov(input_result_location, a_value));

			if (!is_unsigned)
			{
				if (type_size == 1) {
					Code.push_back(Builder::Build_Inst(I_CBW));
				}
				else if (type_size == 2) {
					Code.push_back(Builder::Build_Inst(I_CWD));
				}
				else if (type_size == 4) {
					Code.push_back(Builder::Build_Inst(I_CDQ));
				}
				else if (type_size == 8) {
					Code.push_back(Builder::Build_Inst(I_CQO));
				}
				else {
					GS_CORE_ASSERT(nullptr);
				}
			}

			if (is_unsigned) {
				Code.push_back(Builder::Div(b_value));
			}
			else {
				Code.push_back(Builder::IDiv(b_value));
			}

			UseRegisterValue(rem_reg_id);
			SetRegisterValue(input_result_location, Register_Value_Type::Register_Value);
		}

		UseRegisterValue(ir_div->RegisterA);
		UseRegisterValue(ir_div->RegisterB);
	}

	void X86_BackEnd::AssembleReturn(IRReturn* ir_return)
	{
		Return_Counter++;
		Return_Encountered = true;

		if (!ir_return->Value) {
			return;
		}

		auto ir_register_value = (IRRegisterValue*)ir_return->Value;

		auto data_register = GetRegisterValue(ir_register_value);
		UseRegisterValue(ir_register_value);

		if (data_register->type != Op_Register) {

			auto temp_register_id = CreateTempRegister(nullptr);

			auto temp_phys_register = Allocate_Register(ir_return->Type, temp_register_id);

			SetRegisterValue(temp_phys_register, temp_register_id);

			Code.push_back(Builder::Mov(temp_phys_register, data_register));

			UseRegisterValue(temp_register_id);

			data_register = temp_phys_register;
		}

		Code.push_back(MoveBasedOnType(ir_return->Type, Builder::De_Reference(Return_Storage_Location, ir_return->Type), data_register));
	}

	void X86_BackEnd::AssembleConstValue(IRCONSTValue* ir_constant)
	{
		if (!TypeSystem::IsFlt(TypeSystem::GetBasic(ir_constant->Type))) {
			SetRegisterValue(Builder::Constant_Integer(*(i64*)ir_constant->Data), Register_Value_Type::Immediate_Value);
		}
		else {
			auto type_size = TypeSystem::GetTypeSize(TypeSystem::GetBasic(ir_constant->Type));

			double data = *(double*)ir_constant->Data;

			SetRegisterValue(Create_Floating_Constant(type_size, data), Register_Value_Type::Memory_Value);
		}
	}

	void X86_BackEnd::AssembleData(IRData* ir_data)
	{
		std::string string;

		for (auto c : ir_data->Data) {
			string.push_back(c);
		}

		data_values[ir_data->ID] = Create_String_Constant(string, ir_data->ID);
	}

	void X86_BackEnd::AssembleDataValue(IRDataValue* ir_data_value)
	{
		auto data_location = data_values[ir_data_value->DataID];

		auto address_register = Allocate_Register(TypeSystem::GetVoidPtr(), CurrentRegister);

		Code.push_back(Builder::Lea(address_register, data_location));

		SetRegisterValue(address_register, Register_Value_Type::Register_Value);
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

	Assembly_Operand* X86_BackEnd::Alloc_Call_StackTop(TypeStorage* type)
	{
		auto type_size = TypeSystem::GetTypeSize(type);

		auto inst = Builder::OpAdd(Builder::Register(RSP), Builder::Constant_Integer(m_Data.Call_Stack_Pointer + 32));

		m_Data.Call_Stack_Pointer += type_size;

		if (m_Data.Call_Stack_Pointer >= m_Data.Stack_Size) {
			m_Data.Call_Stack_Size = m_Data.Call_Stack_Pointer;
		}

		return inst;
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

		if (!TypeSystem::IsFlt(type)) {
			if (type_size > 8) {
				return Builder::Register(RAX);
			}
			else {
				return Builder::Register(return_registers.at(type_size));
			}
		}
		else {
			return Builder::Register(XMM0);
		}
	}

	Assembly_Instruction X86_BackEnd::MoveBasedOnType(TypeStorage* type, Assembly_Operand* op1, Assembly_Operand* op2)
	{
		GS_CORE_ASSERT(type);

		auto type_size = TypeSystem::GetTypeSize(type);

		Assembly_Instruction instruction = {};
		instruction.Operand1 = op1;
		instruction.Operand2 = op2;

		if (TypeSystem::IsFlt(type)) {
			if (type_size == 4) {
				instruction.OpCode = I_MovSS;
			}
			else if (type_size == 8) {
				instruction.OpCode = I_MovSD;
			}
			else {
				GS_CORE_ASSERT(nullptr);
			}
		}
		else {
			instruction.OpCode = I_Mov;
		}

		return instruction;
	}

	void X86_BackEnd::SetRegisterValue(Assembly_Operand* register_value, Register_Value_Type value_type)
	{
		SetRegisterValue(register_value, CurrentRegister, value_type);
	}

	void X86_BackEnd::SetRegisterValue(Assembly_Operand* register_value, u64 register_id)
	{
		SetRegisterValue(register_value, register_id, Register_Value_Type::Memory_Value);
	}

	void X86_BackEnd::SetRegisterValue(Assembly_Operand* register_value, u64 register_id, Register_Value_Type value_type)
	{
		m_Data.IR_RegisterValues[register_id] = register_value;
		m_Data.IR_RegisterLifetimes[register_id] = 1;
		m_Data.IR_RegisterValueTypes[register_id] = value_type;
	}

	u64 X86_BackEnd::CreateTempRegister(Assembly_Operand* register_value)
	{
		auto tmp_register_id = Temporary_Register_ID_Counter++;

		m_Data.IR_RegisterValues[tmp_register_id] = register_value;
		m_Data.IR_RegisterLifetimes[tmp_register_id] = 1;

		return tmp_register_id;
	}

	Assembly_Operand* X86_BackEnd::GetRegisterValue(u64 ir_register)
	{
		return m_Data.IR_RegisterValues.at(ir_register);
	}

	Assembly_Operand* X86_BackEnd::GetRegisterValue(IRRegisterValue* ir_register)
	{
		return m_Data.IR_RegisterValues.at(ir_register->RegisterID);
	}

	Register_Value_Type X86_BackEnd::GetRegisterValueType(IRRegisterValue* ir_register)
	{
		return GetRegisterValueType(ir_register->RegisterID);
	}

	Register_Value_Type X86_BackEnd::GetRegisterValueType(u64 register_id)
	{
		return m_Data.IR_RegisterValueTypes.at(register_id);
	}

	void X86_BackEnd::UseRegisterValue(IRRegisterValue* ir_register)
	{
		return UseRegisterValue(ir_register->RegisterID);
	}

	void X86_BackEnd::UseRegisterValue(u64 ir_register)
	{
		Assembly_Operand* value = m_Data.IR_RegisterValues.at(ir_register);

		if (Register_Allocator_Data.allocations.find(ir_register) != Register_Allocator_Data.allocations.end()) {

			auto& liveness = m_Data.IR_RegisterLifetimes.at(ir_register);

			liveness--;

			if (liveness == 0) {
				Register_Allocation& allocation = Register_Allocator_Data.allocations.at(ir_register);
				if (!TypeSystem::IsFlt(allocation.type)) {
					Register_Allocator_Data.allocated.at(allocation.family) = false;
				}
				else {
					Register_Allocator_Data.allocated_floating.at(allocation.family) = false;
				}
			}
		}
	}

	const std::map<std::pair<u64, X86_Register_Family>, X86_Register> register_family_map = {
		{{1,F_A},AL},
		{{2,F_A},AX},
		{{4,F_A},EAX},
		{{8,F_A},RAX},

		{{1,F_B},BL},
		{{2,F_B},BX},
		{{4,F_B},EBX},
		{{8,F_B},RBX},

		{{1,F_C},CL},
		{{2,F_C},CX},
		{{4,F_C},ECX},
		{{8,F_C},RCX},

		{{1,F_D},DL},
		{{2,F_D},DX},
		{{4,F_D},EDX},
		{{8,F_D},RDX},

		{{1,F_R8},R8b},
		{{2,F_R8},R8w},
		{{4,F_R8},R8d},
		{{8,F_R8},R8},

		{{1,F_R9},R9b},
		{{2,F_R9},R9w},
		{{4,F_R9},R9d},
		{{8,F_R9},R9},

		{{1,F_R10},R10b},
		{{2,F_R10},R10w},
		{{4,F_R10},R10d},
		{{8,F_R10},R10},

		{{1,F_R11},R11b},
		{{2,F_R11},R11w},
		{{4,F_R11},R11d},
		{{8,F_R11},R11},

		{{1,F_R12},R12b},
		{{2,F_R12},R12w},
		{{4,F_R12},R12d},
		{{8,F_R12},R12},

		{{1,F_R13},R13b},
		{{2,F_R13},R13w},
		{{4,F_R13},R13d},
		{{8,F_R13},R13},

		{{1,F_R14},R14b},
		{{2,F_R14},R14w},
		{{4,F_R14},R14d},
		{{8,F_R14},R14},

		{{1,F_R15},R15b},
		{{2,F_R15},R15w},
		{{4,F_R15},R15d},
		{{8,F_R15},R15},
	};

	const std::map<X86_Register_Family, X86_Register> register_floating_family_map = {
		{F_X0,XMM0},
		{F_X1,XMM1},
		{F_X2,XMM2},
		{F_X3,XMM3},
		{F_X4,XMM4},
		{F_X5,XMM5},
		{F_X6,XMM6},
		{F_X7,XMM7},
	};

	const std::map<X86_Register, X86_Register_Family> register_to_family_map = {
		{AL,F_A},
		{AX,F_A},
		{EAX,F_A},
		{RAX,F_A},

		{BL,F_B},
		{BX,F_B},
		{EBX,F_B},
		{RBX,F_B},

		{CL,F_C},
		{CX,F_C},
		{ECX,F_C},
		{RCX,F_C},

		{DL,F_D},
		{DX,F_D},
		{EDX,F_D},
		{RDX,F_D},

		{R8b,F_R8},
		{R8w,F_R8},
		{R8d,F_R8},
		{R8,F_R8},

		{R9b,F_R9},
		{R9w,F_R9},
		{R9d,F_R9},
		{R9,F_R9},

		{R10b,F_R10},
		{R10w,F_R10},
		{R10d,F_R10},
		{R10,F_R10},

		{R11b,F_R11},
		{R11w,F_R11},
		{R11d,F_R11},
		{R11,F_R11},

		{R12b,F_R12},
		{R12w,F_R12},
		{R12d,F_R12},
		{R12,F_R12},

		{R13b,F_R13},
		{R13w,F_R13},
		{R13d,F_R13},
		{R13,F_R13},

		{R14b,F_R14},
		{R14w,F_R14},
		{R14d,F_R14},
		{R14,F_R14},

		{R15b,F_R15},
		{R15w,F_R15},
		{R15d,F_R15},
		{R15,F_R15},

		{XMM0,F_X0},
		{XMM1,F_X1},
		{XMM2,F_X2},
		{XMM3,F_X3},
		{XMM4,F_X4},
		{XMM5,F_X5},
		{XMM6,F_X6},
		{XMM7,F_X7},
	};

	Assembly_Operand* X86_BackEnd::Allocate_Register(TypeStorage* type, u64 ir_register)
	{
		GS_CORE_ASSERT(type);

		auto type_size = TypeSystem::GetTypeSize(type);
		GS_CORE_ASSERT(type_size <= 8);

		if (TypeSystem::IsFlt(type)) {
			return Allocate_Float_Register(type, ir_register);
		}

		for (auto& [family, used] : Register_Allocator_Data.allocated) {
			if (!used) {
				used = true;

				X86_Register physical_register = register_family_map.at({ type_size, family });;

				Register_Allocation allocation = { };

				allocation.family = family;
				allocation.reg = physical_register;
				allocation.type = type;
				allocation.virtual_register_id = ir_register;

				Register_Allocator_Data.allocations[ir_register] = allocation;
				Register_Allocator_Data.family_to_allocation[family] = &Register_Allocator_Data.allocations[ir_register];

				return Builder::Register(allocation.reg);
			}
		}

		for (auto [family, used] : Register_Allocator_Data.allocated) {

			auto allocation = Register_Allocator_Data.family_to_allocation[family];

			auto spillage_location = Builder::De_Reference(Stack_Alloc(allocation->type), allocation->type);

			auto spill = Builder::Mov(spillage_location, Builder::Register(allocation->reg));

			spill.Comment = "Spillage";

			Code.push_back(spill);

			m_Data.IR_RegisterValues.at(allocation->virtual_register_id) = spillage_location;
			m_Data.IR_RegisterValueTypes.at(allocation->virtual_register_id) = Register_Value_Type::Memory_Value;

			allocation->spillage_location = spillage_location;

			Register_Allocation new_allocation = { };

			new_allocation.family = family;
			new_allocation.reg = register_family_map.at({ type_size, family });
			new_allocation.type = type;
			new_allocation.virtual_register_id = ir_register;

			Register_Allocator_Data.allocations[ir_register] = new_allocation;
			Register_Allocator_Data.family_to_allocation[family] = &Register_Allocator_Data.allocations[ir_register];

			return Builder::Register(new_allocation.reg);
		}

		return nullptr;
	}

	Assembly_Operand* X86_BackEnd::Allocate_Register(TypeStorage* type, u64 ir_register, X86_Register x86_register)
	{
		GS_CORE_ASSERT(type);

		auto type_size = TypeSystem::GetTypeSize(type);
		GS_CORE_ASSERT(type_size <= 8);

		if (TypeSystem::IsFlt(type)) {
			return Allocate_Float_Register(type, ir_register, x86_register);
		}

		auto family = register_to_family_map.at(x86_register);

		bool& used = Register_Allocator_Data.allocated.at(family);

		if (!used) {
			used = true;

			Register_Allocation allocation = { };

			allocation.family = family;
			allocation.reg = x86_register;
			allocation.type = type;
			allocation.virtual_register_id = ir_register;

			Register_Allocator_Data.allocations[ir_register] = allocation;
			Register_Allocator_Data.family_to_allocation[family] = &Register_Allocator_Data.allocations[ir_register];

			return Builder::Register(allocation.reg);
		}
		else {
			auto allocation = Register_Allocator_Data.family_to_allocation.at(family);

			auto spillage_location = Builder::De_Reference(Stack_Alloc(allocation->type), allocation->type);

			auto spill = MoveBasedOnType(allocation->type, spillage_location, Builder::Register(allocation->reg));

			spill.Comment = "Spillage";

			Code.push_back(spill);

			m_Data.IR_RegisterValues.at(allocation->virtual_register_id) = spillage_location;
			m_Data.IR_RegisterValueTypes.at(allocation->virtual_register_id) = Register_Value_Type::Memory_Value;

			allocation->spillage_location = spillage_location;

			Register_Allocation new_allocation = { };

			new_allocation.family = family;
			new_allocation.reg = x86_register;
			new_allocation.type = type;
			new_allocation.virtual_register_id = ir_register;

			Register_Allocator_Data.allocations[ir_register] = new_allocation;
			Register_Allocator_Data.family_to_allocation[family] = &Register_Allocator_Data.allocations[ir_register];

			return Builder::Register(new_allocation.reg);
		}
	}

	Assembly_Operand* X86_BackEnd::Allocate_Float_Register(TypeStorage* type, u64 ir_register)
	{
		GS_CORE_ASSERT(type);

		auto type_size = TypeSystem::GetTypeSize(type);
		GS_CORE_ASSERT(type_size <= 8);

		for (auto& [family, used] : Register_Allocator_Data.allocated_floating) {
			if (!used) {
				used = true;

				X86_Register physical_register = register_floating_family_map.at(family);

				Register_Allocation allocation = { };

				allocation.family = family;
				allocation.reg = physical_register;
				allocation.type = type;
				allocation.virtual_register_id = ir_register;

				Register_Allocator_Data.allocations[ir_register] = allocation;
				Register_Allocator_Data.family_to_allocation[family] = &Register_Allocator_Data.allocations[ir_register];

				return Builder::Register(allocation.reg);
			}
		}

		for (auto [family, used] : Register_Allocator_Data.allocated_floating) {

			auto allocation = Register_Allocator_Data.family_to_allocation[family];

			auto spillage_location = Stack_Alloc(allocation->type);
			spillage_location = Builder::De_Reference(spillage_location, allocation->type);

			auto spill = MoveBasedOnType(allocation->type, spillage_location, Builder::Register(allocation->reg));

			spill.Comment = "Spillage";

			Code.push_back(spill);

			m_Data.IR_RegisterValues.at(allocation->virtual_register_id) = spillage_location;
			m_Data.IR_RegisterValueTypes.at(allocation->virtual_register_id) = Register_Value_Type::Memory_Value;

			allocation->spillage_location = spillage_location;

			Register_Allocation new_allocation = { };

			new_allocation.family = family;
			new_allocation.reg = register_floating_family_map.at(family);
			new_allocation.type = type;
			new_allocation.virtual_register_id = ir_register;

			Register_Allocator_Data.allocations[ir_register] = new_allocation;
			Register_Allocator_Data.family_to_allocation[family] = &Register_Allocator_Data.allocations[ir_register];

			return Builder::Register(new_allocation.reg);
		}

		return nullptr;
	}

	Assembly_Operand* X86_BackEnd::Allocate_Float_Register(TypeStorage* type, u64 ir_register, X86_Register x86_register)
	{
		GS_CORE_ASSERT(type);

		auto type_size = TypeSystem::GetTypeSize(type);
		GS_CORE_ASSERT(type_size <= 8);

		auto family = register_to_family_map.at(x86_register);

		bool& used = Register_Allocator_Data.allocated_floating.at(family);

		if (!used) {
			used = true;

			Register_Allocation allocation = { };

			allocation.family = family;
			allocation.reg = x86_register;
			allocation.type = type;
			allocation.virtual_register_id = ir_register;

			Register_Allocator_Data.allocations[ir_register] = allocation;
			Register_Allocator_Data.family_to_allocation[family] = &Register_Allocator_Data.allocations[ir_register];

			return Builder::Register(allocation.reg);
		}
		else {
			auto allocation = Register_Allocator_Data.family_to_allocation[family];

			auto spillage_location = Stack_Alloc(allocation->type);
			spillage_location = Builder::De_Reference(spillage_location, allocation->type);

			auto spill = MoveBasedOnType(allocation->type, spillage_location, Builder::Register(allocation->reg));

			spill.Comment = "Spillage";

			Code.push_back(spill);

			m_Data.IR_RegisterValues.at(allocation->virtual_register_id) = spillage_location;
			m_Data.IR_RegisterValueTypes.at(allocation->virtual_register_id) = Register_Value_Type::Memory_Value;

			allocation->spillage_location = spillage_location;

			Register_Allocation new_allocation = { };

			new_allocation.family = family;
			new_allocation.reg = x86_register;
			new_allocation.type = type;
			new_allocation.virtual_register_id = ir_register;

			Register_Allocator_Data.allocations[ir_register] = new_allocation;
			Register_Allocator_Data.family_to_allocation[family] = &Register_Allocator_Data.allocations[ir_register];

			return Builder::Register(new_allocation.reg);
		}
	}

	Assembly_Operand* X86_BackEnd::Create_Floating_Constant(u64 size, double value)
	{
		Float_Constant_Counter++;

		Assembly_Float_Constant constant;
		constant.index = Float_Constant_Counter;
		constant.size = size;
		constant.value = value;

		Floats.push_back(constant);

		return Builder::De_Reference(Builder::Symbol(fmt::format("fl_{}", Float_Constant_Counter)));
	}

	Assembly_Operand* X86_BackEnd::Create_String_Constant(const std::string& data, u64 id)
	{
		Assembly_String_Constant constant;

		constant.id = id;
		constant.value = data;

		Strings.push_back(constant);

		return Builder::De_Reference(Builder::Symbol(fmt::format("str_{}", id)));
	}

	bool X86_BackEnd::Are_Equal(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		if (operand1->type != operand2->type) {
			return false;
		}

		if (operand1->type == Op_Register) {
			if (operand1->reg.Register == operand2->reg.Register) {
				return true;
			}
		}

		return false;
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

	std::string doubleToHexString(double value) {
		std::ostringstream stream;
		stream << std::setprecision(15) << std::hexfloat << value;
		std::string hexString = stream.str();

		return hexString;
	}

	std::string floatToHexString(float value) {
		std::ostringstream stream;
		stream << std::setprecision(8) << std::hexfloat << value;
		std::string hexString = stream.str();

		return hexString;
	}

	std::string FASM_Printer::Print()
	{
		std::stringstream stream;

		stream << "format MS64 COFF" << "\n" << "\n";
		stream << "public main" << "\n" << "\n";

		for (Assembly_External_Symbol external : Assembly->externals) {
			stream << "extrn '" << external.ExternalName << "' " << "as " << external.ExternalName << "\n";
		}

		//data
		stream << "\n";
		stream << "section '.rdata' data readable\n";

		for (Assembly_Float_Constant& floating_constant : Assembly->floats) {

			stream << "fl_";
			stream << floating_constant.index;
			stream << " ";

			if (floating_constant.size == 4) {
				float data = (float)floating_constant.value;
				stream << "dd ";
				stream << fmt::format("0x{0:x}", *(u32*)&data);
				stream << " ; ";
				stream << fmt::format("{}", data);
			}
			else if (floating_constant.size == 8) {
				double data = (double)floating_constant.value;
				stream << "dq ";
				stream << fmt::format("0x{0:x}", *(u64*)&data);
				stream << " ; ";
				stream << fmt::format("{}", data);
			}
			else {
				GS_CORE_ASSERT(nullptr);
			}
			stream << '\n';

		}

		for (Assembly_String_Constant& constant_string : Assembly->strings) {

			stream << "str_";
			stream << constant_string.id;
			stream << " db \"";

			for (size_t i = 0; i < constant_string.value.size(); i++)
			{
				char c = constant_string.value[i];

				if (c == '\\') {

					if (constant_string.value[i + 1] == 'n') {
						stream << "\"";

						stream << ", 0ah, ";

						stream << "\"";
						i++;
						continue;
					}
				}
				else {
					stream << c;
				}
			}

			stream << "\", 0\n";
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

		case Op_Add:
			PrintOperand(operand->bin_op.operand1, stream);
			stream << " + ";
			PrintOperand(operand->bin_op.operand2, stream);
			break;

		case Op_De_Reference:
			stream << wordness_map.at(operand->de_reference.wordness);
			stream << '[';
			PrintOperand(operand->de_reference.operand, stream);
			stream << ']';
			break;
		case Op_Symbol:
			stream << operand->symbol.symbol;
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
		case I_AddSS:
			stream << "addss ";
			PrintOperand(instruction.Operand1, stream);
			stream << ", ";
			PrintOperand(instruction.Operand2, stream);
			break;
		case I_AddSD:
			stream << "addsd ";
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
		case I_SubSS:
			stream << "subss ";
			PrintOperand(instruction.Operand1, stream);
			stream << ", ";
			PrintOperand(instruction.Operand2, stream);
			break;
		case I_SubSD:
			stream << "subsd ";
			PrintOperand(instruction.Operand1, stream);
			stream << ", ";
			PrintOperand(instruction.Operand2, stream);
			break;

		case I_IDiv:
			stream << "idiv ";
			PrintOperand(instruction.Operand1, stream);
			break;

		case I_Div:
			stream << "div ";
			PrintOperand(instruction.Operand1, stream);
			break;

		case I_IMul:
			stream << "imul ";
			PrintOperand(instruction.Operand1, stream);
			stream << ", ";
			PrintOperand(instruction.Operand2, stream);
			break;
		case I_MulSS:
			stream << "mulss ";
			PrintOperand(instruction.Operand1, stream);
			stream << ", ";
			PrintOperand(instruction.Operand2, stream);
			break;
		case I_MulSD:
			stream << "mulsd ";
			PrintOperand(instruction.Operand1, stream);
			stream << ", ";
			PrintOperand(instruction.Operand2, stream);
			break;

		case I_DivSS:
			stream << "divss ";
			PrintOperand(instruction.Operand1, stream);
			stream << ", ";
			PrintOperand(instruction.Operand2, stream);
			break;
		case I_DivSD:
			stream << "divsd ";
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

		case I_MovD:
			stream << "movd ";
			PrintOperand(instruction.Operand1, stream);
			stream << ", ";
			PrintOperand(instruction.Operand2, stream);
			break;

		case I_MovQ:
			stream << "movq ";
			PrintOperand(instruction.Operand1, stream);
			stream << ", ";
			PrintOperand(instruction.Operand2, stream);
			break;

		case I_MovSS:
			stream << "movss ";
			PrintOperand(instruction.Operand1, stream);
			stream << ", ";
			PrintOperand(instruction.Operand2, stream);
			break;

		case I_MovSD:
			stream << "movsd ";
			PrintOperand(instruction.Operand1, stream);
			stream << ", ";
			PrintOperand(instruction.Operand2, stream);
			break;

		case I_Lea:
			stream << "lea ";
			PrintOperand(instruction.Operand1, stream);
			stream << ", ";
			PrintOperand(instruction.Operand2, stream);
			break;

		case I_CvtSS2SD:
			stream << "cvtss2sd ";
			PrintOperand(instruction.Operand1, stream);
			stream << ", ";
			PrintOperand(instruction.Operand2, stream);
			break;

		case I_Call:
			stream << "call ";
			PrintOperand(instruction.Operand1, stream);
			break;

		case I_CBW:
			stream << "cbw";
			break;
		case I_CWD:
			stream << "cwd";
			break;
		case I_CDQ:
			stream << "cdq";
			break;
		case I_CQO:
			stream << "cqo";
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

				if (code.Comment) {
					stream << "\t; ";
					stream << code.Comment;
				}

				stream << "\n";
			}
			stream << "\n";
			stream << "\n";
		}
	}
}