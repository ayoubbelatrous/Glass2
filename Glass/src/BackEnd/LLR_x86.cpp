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

	Assembly_Instruction Builder::Mul(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_IMul;
		instruction.Operand1 = operand1;
		instruction.Operand2 = operand2;

		return instruction;
	}

	Assembly_Instruction Builder::Div(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_IDiv;
		instruction.Operand1 = operand1;
		instruction.Operand2 = operand2;

		return instruction;
	}

	Assembly_Instruction Builder::Call(Assembly_Operand* operand1)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_Call;
		instruction.Operand1 = operand1;

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

		{X86_Register::AX,"ax"},
		{X86_Register::BX,"bx"},
		{X86_Register::CX,"cx"},

		{X86_Register::EAX,"eax"},
		{X86_Register::EBX,"ebx"},
		{X86_Register::ECX,"ecx"},

		{X86_Register::RAX,"rax"},
		{X86_Register::RBX,"rbx"},
		{X86_Register::RCX,"rcx"},

		{X86_Register::XMM0,"xmm0"},
		{X86_Register::XMM1,"xmm1"},

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
		};

		Register_Allocator_Data.allocated_floating = {
			{F_X0, false},
			{F_X1, false},
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
		system("clang .build/fasm.obj");
		std::chrono::steady_clock::time_point Linker_End = std::chrono::high_resolution_clock::now();

		GS_CORE_WARN("Assembly Generation Took: {} micro s", std::chrono::duration_cast<std::chrono::microseconds>(End - Start).count());
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

		{ {4,0}, ECX},
		{ {4,1}, EDX},

		{ {2,0}, CX},
		{ {2,1}, DX},

		{ {1,0}, CL},
		{ {1,1}, DL},
	};

	void X86_BackEnd::AssembleFunction(IRFunction* ir_function)
	{
		const auto& metadata = m_Metadata->GetFunctionMetadata(ir_function->ID);

		Code.clear();

		m_Data.IR_RegisterValues.clear();
		m_Data.IR_RegisterTypes.clear();
		m_Data.Stack_Size = 0;

		Return_Storage_Location = nullptr;
		Return_Counter = 0;
		Return_Encountered = false;

		Register_Allocator_Data.allocations.clear();
		Register_Allocator_Data.family_to_allocation.clear();

		Assembly_Function* assembly = m_Data.Functions[ir_function->ID];

		Assembly_Operand* stack_size_constant = Builder::Constant_Integer(32);

		Code.push_back(Builder::Push(Builder::Register(RBP)));
		Code.push_back(Builder::Sub(Builder::Register(RSP), stack_size_constant));

		if (metadata->ReturnType != TypeSystem::GetVoid()) {
			Return_Storage_Location = Stack_Alloc(metadata->ReturnType);
		}

		u64 i = 0;

		for (const ArgumentMetadata& argument : metadata->Arguments) {
			auto type_size = TypeSystem::GetTypeSize(argument.Type);

			if (type_size <= 8) {

				auto needed_register = argument_register_map.at({ type_size, i });

				CurrentRegister = argument.AllocationLocation->RegisterID;

				SetRegisterValue(Allocate_Register(argument.Type, CurrentRegister, needed_register), Register_Liveness::Value);
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

		for (size_t i = 0; i < ir_function->Arguments.size(); i++)
		{
			auto call_argument = (IRRegisterValue*)ir_function->Arguments[i];
			const ArgumentMetadata& argument_metadata = metadata->Arguments[i];

			auto type_size = TypeSystem::GetTypeSize(argument_metadata.Type);

			if (type_size <= 8) {

				auto needed_register = argument_register_map.at({ type_size,i });

				auto temp_register_id = CreateTempRegister(nullptr);
				auto temp_phys_register = Allocate_Register(argument_metadata.Type, temp_register_id, needed_register);

				SetRegisterValue(temp_phys_register, temp_register_id);

				auto call_argument_value = GetRegisterValue(call_argument);

				if (GetRegisterLiveness(call_argument) == Register_Liveness::Address_To_Value) {
					call_argument_value = Builder::De_Reference(call_argument_value, argument_metadata.Type);
				}

				Code.push_back(Builder::Mov(temp_phys_register, call_argument_value));

				UseRegisterValue(call_argument);

				UseRegisterValue(temp_register_id);
			}
			else {
				GS_CORE_ASSERT(nullptr);
			}
		}


		if (metadata->ReturnType != TypeSystem::GetVoid()) {
			auto return_type_size = TypeSystem::GetTypeSize(metadata->ReturnType);
			Assembly_Operand* return_value = nullptr;
			return_value = Allocate_Register(metadata->ReturnType, CurrentRegister, return_register_map.at(return_type_size));
			SetRegisterValue(return_value, Register_Liveness::Value);
		}


		Code.push_back(Builder::Call(Builder::Symbol(name)));
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

		Code.push_back(Builder::Mov(Builder::De_Reference(argument_storage_location, ir_argument->AllocationType), argument_input_location));

		SetRegisterValue(argument_storage_location, Register_Liveness::Address_To_Value);

		m_Data.IR_RegisterLifetimes.at(CurrentRegister) = 1;
	}

	void X86_BackEnd::AssembleAlloca(IRAlloca* ir_alloca)
	{
		auto register_value = Stack_Alloc(ir_alloca->Type);
		SetRegisterValue(register_value, Register_Liveness::Address_To_Value);
	}

	void X86_BackEnd::AssembleStore(IRStore* ir_store)
	{
		auto type_size = TypeSystem::GetTypeSize(ir_store->Type);

		auto pointer_register_value = GetRegisterValue(ir_store->AddressRegister);
		UseRegisterValue(ir_store->AddressRegister);

		auto data_register_value = GetRegisterValue((IRRegisterValue*)ir_store->Data);
		UseRegisterValue((IRRegisterValue*)ir_store->Data);

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
	}

	void X86_BackEnd::AssembleLoad(IRLoad* ir_load)
	{
		auto type_size = TypeSystem::GetTypeSize(ir_load->Type);

		auto pointer_register_value = GetRegisterValue(ir_load->AddressRegister);
		UseRegisterValue(ir_load->AddressRegister);

		if (type_size <= 8) {
			auto loaded_data_register = Allocate_Register(ir_load->Type, CurrentRegister);
			Code.push_back(MoveBasedOnType(ir_load->Type, loaded_data_register, Builder::De_Reference(pointer_register_value, ir_load->Type)));
			SetRegisterValue(loaded_data_register, Register_Liveness::Value);
		}
		else {
			GS_CORE_ASSERT(nullptr);
		}
	}

	void X86_BackEnd::AssembleAdd(IRADD* ir_add)
	{
		auto result_location = Allocate_Register(ir_add->Type, CurrentRegister);

		auto a_value = GetRegisterValue(ir_add->RegisterA);
		UseRegisterValue(ir_add->RegisterA);
		auto b_value = GetRegisterValue(ir_add->RegisterB);
		UseRegisterValue(ir_add->RegisterB);

		if (!Are_Equal(result_location, a_value)) {

			if (a_value->type != Op_Register && a_value->type != Op_De_Reference) {
				a_value = Builder::De_Reference(a_value, ir_add->Type);
			}

			Code.push_back(MoveBasedOnType(ir_add->Type, result_location, a_value));
		}

		SetRegisterValue(result_location, Register_Liveness::Value);
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
	}

	void X86_BackEnd::AssembleSub(IRSUB* ir_sub)
	{
		auto result_location = Allocate_Register(ir_sub->Type, CurrentRegister);

		auto a_value = GetRegisterValue(ir_sub->RegisterA);
		UseRegisterValue(ir_sub->RegisterA);
		auto b_value = GetRegisterValue(ir_sub->RegisterB);
		UseRegisterValue(ir_sub->RegisterB);

		if (!Are_Equal(result_location, a_value)) {

			if (a_value->type != Op_Register && a_value->type != Op_De_Reference) {
				a_value = Builder::De_Reference(a_value, ir_sub->Type);
			}

			Code.push_back(Builder::Mov(result_location, a_value));
		}

		SetRegisterValue(result_location, Register_Liveness::Value);

		Code.push_back(Builder::Sub(result_location, b_value));
	}

	void X86_BackEnd::AssembleMul(IRMUL* ir_mul)
	{
		auto result_location = Allocate_Register(ir_mul->Type, CurrentRegister);

		auto a_value = GetRegisterValue(ir_mul->RegisterA);
		UseRegisterValue(ir_mul->RegisterA);
		auto b_value = GetRegisterValue(ir_mul->RegisterB);
		UseRegisterValue(ir_mul->RegisterB);

		if (!Are_Equal(result_location, a_value)) {

			if (a_value->type != Op_Register && a_value->type != Op_De_Reference) {
				a_value = Builder::De_Reference(a_value, ir_mul->Type);
			}

			Code.push_back(Builder::Mov(result_location, a_value));
		}

		SetRegisterValue(result_location, Register_Liveness::Value);

		Code.push_back(Builder::Mul(result_location, b_value));
	}

	void X86_BackEnd::AssembleDiv(IRDIV* ir_div)
	{

		auto result_location = Allocate_Register(ir_div->Type, CurrentRegister);

		auto a_value = GetRegisterValue(ir_div->RegisterA);
		UseRegisterValue(ir_div->RegisterA);
		auto b_value = GetRegisterValue(ir_div->RegisterB);
		UseRegisterValue(ir_div->RegisterB);

		if (!Are_Equal(result_location, a_value)) {

			if (a_value->type != Op_Register && a_value->type != Op_De_Reference) {
				a_value = Builder::De_Reference(a_value, ir_div->Type);
			}

			Code.push_back(Builder::Mov(result_location, a_value));
		}

		SetRegisterValue(result_location, Register_Liveness::Value);

		Code.push_back(Builder::Div(result_location, b_value));
	}

	void X86_BackEnd::AssembleReturn(IRReturn* ir_return)
	{
		Return_Counter++;
		Return_Encountered = true;

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
		if (!(ir_constant->Type & FLAG_FLOATING_TYPE)) {
			SetRegisterValue(Builder::Constant_Integer(*(i64*)ir_constant->Data), Register_Liveness::Value);
		}
		else {
			auto type_size = TypeSystem::GetTypeSize(TypeSystem::GetBasic(ir_constant->Type));

			double data = *(double*)ir_constant->Data;

			SetRegisterValue(Create_Floating_Constant(type_size, data), Register_Liveness::Value);
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

	void X86_BackEnd::SetRegisterValue(Assembly_Operand* register_value, Register_Liveness liveness)
	{
		SetRegisterValue(register_value, CurrentRegister, liveness);
	}

	void X86_BackEnd::SetRegisterValue(Assembly_Operand* register_value, u64 register_id)
	{
		SetRegisterValue(register_value, register_id, Register_Liveness::Other);
	}

	void X86_BackEnd::SetRegisterValue(Assembly_Operand* register_value, u64 register_id, Register_Liveness liveness)
	{
		m_Data.IR_RegisterValues[register_id] = register_value;
		m_Data.IR_RegisterLifetimes[register_id] = 1;
		m_Data.IR_RegisterLiveness[register_id] = liveness;
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

	Register_Liveness X86_BackEnd::GetRegisterLiveness(IRRegisterValue* ir_register)
	{
		return m_Data.IR_RegisterLiveness.at(ir_register->RegisterID);
	}

	void X86_BackEnd::UseRegisterValue(IRRegisterValue* ir_register)
	{
		return UseRegisterValue(ir_register->RegisterID);
	}

	void X86_BackEnd::UseRegisterValue(u64 ir_register)
	{
		Assembly_Operand* value = m_Data.IR_RegisterValues.at(ir_register);

		if (value->type != Op_Register) {
			return;
		}

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
	};

	const std::map<X86_Register_Family, X86_Register> register_floating_family_map = {
		{F_X0,XMM0},
		{F_X1,XMM1},
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

		{XMM0,F_X0},
		{XMM1,F_X1},
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

			auto spillage_location = Stack_Alloc(allocation->type);

			auto spill = Builder::Mov(Builder::De_Reference(spillage_location, allocation->type), Builder::Register(allocation->reg));

			spill.Comment = "Spillage";

			Code.push_back(spill);

			m_Data.IR_RegisterValues.at(allocation->virtual_register_id) = spillage_location;
			m_Data.IR_RegisterLiveness.at(allocation->virtual_register_id) = Register_Liveness::Address_To_Value;

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
			auto allocation = Register_Allocator_Data.family_to_allocation[family];

			auto spillage_location = Stack_Alloc(allocation->type);

			auto spill = MoveBasedOnType(allocation->type, Builder::De_Reference(spillage_location, allocation->type), Builder::Register(allocation->reg));

			spill.Comment = "Spillage";

			Code.push_back(spill);

			m_Data.IR_RegisterValues.at(allocation->virtual_register_id) = spillage_location;
			m_Data.IR_RegisterLiveness.at(allocation->virtual_register_id) = Register_Liveness::Address_To_Value;

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

			auto spill = MoveBasedOnType(allocation->type, Builder::De_Reference(spillage_location, allocation->type), Builder::Register(allocation->reg));

			spill.Comment = "Spillage";

			Code.push_back(spill);

			m_Data.IR_RegisterValues.at(allocation->virtual_register_id) = spillage_location;
			m_Data.IR_RegisterLiveness.at(allocation->virtual_register_id) = Register_Liveness::Address_To_Value;

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
			}
			else if (floating_constant.size == 8) {
				double data = (double)floating_constant.value;
				stream << "dq ";
				stream << fmt::format("0x{0:x}", *(u64*)&data);
			}
			else {
				GS_CORE_ASSERT(nullptr);
			}
			stream << '\n';

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
		case I_IDiv:
			stream << "idiv ";
			PrintOperand(instruction.Operand1, stream);
			stream << ", ";
			PrintOperand(instruction.Operand2, stream);
			break;
		case I_IMul:
			stream << "imul ";
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

		case I_Call:
			stream << "call ";
			PrintOperand(instruction.Operand1, stream);
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
		}
	}
}