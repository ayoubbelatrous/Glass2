#pragma once

#include "BackEnd/IR.h"
#include "BackEnd/Metadata.h"

namespace Glass
{
	struct Assembly_External_Symbol {
		std::string ExternalName;
		std::string Name;
	};

	enum Assembly_Op_Code {
		I_Ret,

		I_Push,
		I_Pop,

		I_Add,
		I_Sub,

		I_IMul,
		I_IDiv,

		I_Div,
		I_Mul,

		I_AddSS,
		I_AddSD,

		I_SubSS,
		I_SubSD,

		I_MulSS,
		I_MulSD,

		I_DivSS,
		I_DivSD,

		I_Mov,
		I_MovD,
		I_MovQ,

		I_MovSS,
		I_MovSD,

		I_MovZX,

		I_MovSX,
		I_MovSXD,

		I_Lea,

		I_CvtSS2SD,
		I_CvtSD2SS,

		I_CBW,
		I_CWD,
		I_CDQ,
		I_CQO,

		I_Cmp,

		I_Setne,
		I_Sete,

		I_Setl,
		I_Setg,

		I_Setle,
		I_Setge,

		I_Je,
		I_Jne,

		I_Jg,
		I_Jl,

		I_Jle,
		I_Jge,

		I_Jmp,

		I_And,
		I_Or,

		I_Call,

		I_Label,
	};

	enum X86_Register_Family {

		F_None,

		F_BP,
		F_SP,

		F_A,
		F_B,

		F_R10,
		F_R11,
		F_R12,
		F_R13,
		F_R14,
		F_R15,

		F_C,
		F_D,
		F_R8,
		F_R9,

		F_X5,
		F_X6,
		F_X7,
		F_X0,
		F_X1,
		F_X2,
		F_X3,
		F_X4,
	};

	enum X86_Register {
		None,

		RBP,
		RSP,

		AL,
		AH,
		BL,
		BH,
		DL,
		DH,
		CL,
		CH,

		AX,
		BX,
		CX,
		DX,

		EAX,
		EBX,
		ECX,
		EDX,

		RAX,
		RBX,
		RCX,
		RDX,

		XMM0,
		XMM1,
		XMM2,
		XMM3,
		XMM4,
		XMM5,
		XMM6,
		XMM7,

		R8b,
		R9b,
		R10b,
		R11b,
		R12b,
		R13b,
		R14b,
		R15b,

		R8w,
		R9w,
		R10w,
		R11w,
		R12w,
		R13w,
		R14w,
		R15w,

		R8d,
		R9d,
		R10d,
		R11d,
		R12d,
		R13d,
		R14d,
		R15d,

		R8,
		R9,
		R10,
		R11,
		R12,
		R13,
		R14,
		R15,
	};

	struct Assembly_Operand;

	enum Assembly_Size {
		asm_none,
		asm_byte,
		asm_word,
		asm_dword,
		asm_qword,
	};

	enum Assembly_Operand_Type {
		Op_Register,
		Op_De_Reference,
		Op_Constant_Integer,
		Op_Symbol,

		Op_Add,
		Op_Sub,
		Op_Mul,
		Op_Div,
	};

	struct Assembly_Operand_Register {
		X86_Register Register;
	};

	struct Assembly_Operand_Constant_Integer {
		i64 integer = 0;
	};

	struct Assembly_Operand_Dereference {
		Assembly_Operand* operand;
		Assembly_Size wordness;
	};

	struct Assembly_Operand_Symbol {
		char* symbol = nullptr;
	};

	struct Assembly_Operand_BinOp {
		Assembly_Operand* operand1 = nullptr;
		Assembly_Operand* operand2 = nullptr;
	};

	struct Assembly_Operand {
		Assembly_Operand_Type type;
		union {
			Assembly_Operand_Register reg;
			Assembly_Operand_Dereference de_reference;
			Assembly_Operand_Constant_Integer constant_integer;
			Assembly_Operand_Symbol symbol;
			Assembly_Operand_BinOp bin_op;
		};
	};

	struct Assembly_Instruction {
		Assembly_Op_Code OpCode;
		Assembly_Operand* Operand1;
		Assembly_Operand* Operand2;
		const char* Comment = nullptr;
	};

	struct Assembly_Function {
		std::string Name;
		Assembly_Operand* Label = nullptr;
		std::vector<Assembly_Instruction> Code;
	};


	struct Assembly_Float_Constant {
		u64 index = 0;
		u64 size = 0;
		double value = 0.0;
	};

	struct Assembly_String_Constant {
		u64 id = 0;
		std::string value;
	};

	struct Assembly_File {
		std::vector<Assembly_External_Symbol> externals;
		std::vector<Assembly_Function> functions;
		std::vector<Assembly_Float_Constant> floats;
		std::vector<Assembly_String_Constant> strings;
	};

	struct FASM_Printer {

		FASM_Printer(Assembly_File* assembly);

		std::string Print();
		void PrintOperand(const Assembly_Operand* operand, std::stringstream& stream);
		void PrintInstruction(const Assembly_Instruction& instruction, std::stringstream& stream);
		void PrintCode(std::stringstream& stream);

		Assembly_File* Assembly = nullptr;
	};

	inline i64 align_to(i64 unaligned_val, i64 alignment) {
		i64 mask = alignment - 1;
		i64 aligned_val = unaligned_val + (-unaligned_val & mask);
		return aligned_val;
	}

	inline Assembly_Size to_asm_size(u64 size) {

		if (size == 1) {
			return asm_byte;
		}
		else if (size == 2) {
			return asm_word;
		}
		else if (size == 4) {
			return asm_dword;
		}
		else if (size == 8) {
			return asm_qword;
		}

		return asm_none;
	}

	struct Builder
	{
		static Assembly_Instruction Ret();

		static Assembly_Instruction Build_Inst(Assembly_Op_Code op_code, Assembly_Operand* op1 = nullptr, Assembly_Operand* op2 = nullptr);

		static Assembly_Instruction Label(Assembly_Operand* operand);

		static Assembly_Instruction Push(Assembly_Operand* operand);
		static Assembly_Instruction Pop(Assembly_Operand* operand);
		static Assembly_Instruction Add(Assembly_Operand* operand1, Assembly_Operand* operand2);
		static Assembly_Instruction AddSS(Assembly_Operand* operand1, Assembly_Operand* operand2);
		static Assembly_Instruction AddSD(Assembly_Operand* operand1, Assembly_Operand* operand2);
		static Assembly_Instruction Sub(Assembly_Operand* operand1, Assembly_Operand* operand2);
		static Assembly_Instruction SubSS(Assembly_Operand* operand1, Assembly_Operand* operand2);
		static Assembly_Instruction SubSD(Assembly_Operand* operand1, Assembly_Operand* operand2);
		static Assembly_Instruction Mul(Assembly_Operand* operand1, Assembly_Operand* operand2);
		static Assembly_Instruction MulSS(Assembly_Operand* operand1, Assembly_Operand* operand2);
		static Assembly_Instruction MulSD(Assembly_Operand* operand1, Assembly_Operand* operand2);
		static Assembly_Instruction IDiv(Assembly_Operand* operand1);
		static Assembly_Instruction Div(Assembly_Operand* operand1);
		static Assembly_Instruction DivSS(Assembly_Operand* operand1, Assembly_Operand* operand2);
		static Assembly_Instruction DivSD(Assembly_Operand* operand1, Assembly_Operand* operand2);
		static Assembly_Instruction Call(Assembly_Operand* operand1);
		static Assembly_Instruction Lea(Assembly_Operand* operand1, Assembly_Operand* operand2);

		static Assembly_Instruction Cmp(Assembly_Operand* operand1, Assembly_Operand* operand2);

		static Assembly_Instruction SetNe(Assembly_Operand* op);
		static Assembly_Instruction SetE(Assembly_Operand* op);

		static Assembly_Instruction SetL(Assembly_Operand* op);
		static Assembly_Instruction SetG(Assembly_Operand* op);

		static Assembly_Instruction And(Assembly_Operand* operand1, Assembly_Operand* operand2);
		static Assembly_Instruction Or(Assembly_Operand* operand1, Assembly_Operand* operand2);

		static Assembly_Instruction SS2SD(Assembly_Operand* operand1, Assembly_Operand* operand2);

		static Assembly_Instruction Mov(Assembly_Operand* operand1, Assembly_Operand* operand2);
		static Assembly_Instruction MovD(Assembly_Operand* operand1, Assembly_Operand* operand2);
		static Assembly_Instruction MovQ(Assembly_Operand* operand1, Assembly_Operand* operand2);

		static Assembly_Instruction MovZX(Assembly_Operand* operand1, Assembly_Operand* operand2);

		static Assembly_Instruction MovSX(Assembly_Operand* operand1, Assembly_Operand* operand2);
		static Assembly_Instruction MovSXD(Assembly_Operand* operand1, Assembly_Operand* operand2);

		static Assembly_Operand* OpAdd(Assembly_Operand* operand1, Assembly_Operand* operand2);
		static Assembly_Operand* OpSub(Assembly_Operand* operand1, Assembly_Operand* operand2);
		static Assembly_Operand* OpMul(Assembly_Operand* operand1, Assembly_Operand* operand2);
		static Assembly_Operand* OpDiv(Assembly_Operand* operand1, Assembly_Operand* operand2);

		static Assembly_Operand* Register(X86_Register reg);
		static Assembly_Operand* Constant_Integer(i64 integer);
		static Assembly_Operand* De_Reference(Assembly_Operand* operand1, TypeStorage* type = nullptr);
		static Assembly_Operand* Symbol(const std::string& symbol);
	};

	struct Register_Allocation {
		u64 virtual_register_id = (u64)-1;
		X86_Register reg;
		X86_Register_Family family;
		TypeStorage* type = nullptr;
		Assembly_Operand* spillage_location = nullptr;
	};

	struct Register_Allocation_Data {
		std::map<X86_Register_Family, bool> allocated;
		std::map<X86_Register_Family, bool> allocated_floating;
		std::unordered_map<u64, Register_Allocation> allocations;
		std::unordered_map<X86_Register_Family, u64> family_to_allocation;
	};

	enum class Register_Value_Type {
		Immediate_Value, // 50, 60 etc
		Register_Value, // rax or 16
		Memory_Value, // qword [rax + 16] or qword [rbp - 16] or dword [flt_60]

		Stack_Address, // rbp - 4
		Pointer_Address, // rbx + 4 rbx
	};

	struct X86_BackEnd_Data
	{
		std::unordered_map<u64, Assembly_Operand*> IR_RegisterValues;
		std::unordered_map<u64, TypeStorage*> IR_RegisterTypes;
		std::unordered_map<u64, u64> IR_RegisterLifetimes;
		std::unordered_map<u64, Register_Value_Type> IR_RegisterValueTypes;
		std::unordered_map<u64, Assembly_Function*> Functions;

		u64 Stack_Size = 0;
		u64 Call_Stack_Size = 0;
		u64 Call_Stack_Pointer = 0;
	};

	class X86_BackEnd
	{
	public:
		X86_BackEnd(IRTranslationUnit* translation_unit, MetaData* metadata);

		void Init();

		std::string Mangle_Name(const std::string& name, TypeStorage* type);

		void AssembleExternalFunction(const FunctionMetadata* function);
		void AssembleExternals();
		void Assemble();

		void AssembleInstruction(IRInstruction* instruction);
		void AssembleFunctionSymbol(IRFunction* ir_function);
		void AssembleFunction(IRFunction* ir_function);

		void AssembleFunctionCall(IRFunctionCall* ir_function);

		void AssembleRegister(IRRegister* ir_register);

		void AssembleArgument(IRArgumentAllocation* ir_argument);

		void AssembleAlloca(IRAlloca* ir_alloca);

		void AssembleMemberAccess(IRMemberAccess* ir_member_access);
		void AssembleArrayAccess(IRArrayAccess* ir_array_access);

		void EndBlock();

		void AssembleLexicalBlock(IRLexBlock* ir_lex_block);
		void AssembleIf(IRIf* ir_if);
		void AssembleWhile(IRWhile* ir_while);

		void AssembleStore(IRStore* ir_store);
		void AssembleLoad(IRLoad* ir_load);

		void AssembleAdd(IRADD* ir_add);
		void AssembleSub(IRSUB* ir_sub);
		void AssembleMul(IRMUL* ir_mul);
		void AssembleDiv(IRDIV* ir_div);

		void AssembleAnd(IRAnd* ir_and);
		void AssembleOr(IROr* ir_or);
		void AssembleEqual(IREQ* ir_eq);

		void AssembleLesser(IRLesser* ir_lesser);
		void AssembleGreater(IRGreater* ir_greater);

		void AssembleBitAnd(IRBitAnd* ir_bit_and);

		void AssemblePointerCast(IRPointerCast* ir_pointer_cast);
		void AssembleInt2PCast(IRInt2PtrCast* ir_int_2_ptr);
		void AssemblePtr2IntCast(IRPtr2IntCast* ir_ptr_2_int);
		void AssembleSextCast(IRSExtCast* ir_sext);
		void AssembleZextCast(IRZExtCast* ir_zext);
		void AssembleIntTruncCast(IRIntTrunc* ir_int_trunc);

		void AssembleReturn(IRReturn* ir_return);

		void AssembleConstValue(IRCONSTValue* ir_constant);

		std::unordered_map<u64, Assembly_Operand*> data_values;

		void AssembleData(IRData* ir_data);
		void AssembleDataValue(IRDataValue* ir_data_value);

		TypeStorage* GetIRNodeType(IRInstruction* ir);

		X86_BackEnd_Data m_Data;
		IRTranslationUnit* m_TranslationUnit = nullptr;
		MetaData* m_Metadata = nullptr;

		std::vector<Assembly_External_Symbol> Externals;
		std::vector<Assembly_String_Constant> Strings;
		std::vector<Assembly_Function*> Functions;
		std::vector<Assembly_Instruction> Code;
		std::vector<Assembly_Float_Constant> Floats;

		std::string GetLabelName();

		Assembly_Operand* Stack_Alloc(TypeStorage* type);
		Assembly_Operand* Alloc_Call_StackTop(TypeStorage* type);
		Assembly_Operand* GetReturnRegister(TypeStorage* type);

		Assembly_Instruction MoveBasedOnType(TypeStorage* type, Assembly_Operand* op1, Assembly_Operand* op2);

		u64 CurrentRegister = 0;
		IRRegister* CurrentIrRegister = nullptr;

		void SetRegisterValue(Assembly_Operand* register_value, Register_Value_Type value_type);
		void SetRegisterValue(Assembly_Operand* register_value, u64 register_id);
		void SetRegisterValue(Assembly_Operand* register_value, u64 register_id, Register_Value_Type value_type);
		u64 CreateTempRegister(Assembly_Operand* register_value);

		Assembly_Operand* GetRegisterValue(u64 ir_register);
		Assembly_Operand* GetRegisterValue(IRRegisterValue* ir_register);
		Register_Value_Type GetRegisterValueType(u64 register_id);
		Register_Value_Type GetRegisterValueType(IRRegisterValue* ir_register);

		void UseRegisterValue(u64 ir_register);
		void UseRegisterValue(IRRegisterValue* ir_register);

		Assembly_Operand* Allocate_Register(TypeStorage* type, u64 ir_register);
		Assembly_Operand* Allocate_Register(TypeStorage* type, u64 ir_register, X86_Register x86_register);

		Assembly_Operand* Allocate_Float_Register(TypeStorage* type, u64 ir_register);
		Assembly_Operand* Allocate_Float_Register(TypeStorage* type, u64 ir_register, X86_Register x86_register);

		Assembly_Operand* Create_Floating_Constant(u64 size, double value);
		Assembly_Operand* Create_String_Constant(const std::string& data, u64 id);

		bool Are_Equal(Assembly_Operand* operand1, Assembly_Operand* operand2);

		Register_Allocation_Data Register_Allocator_Data;

		Assembly_Operand* Return_Storage_Location = nullptr;
		u64 Return_Counter = 0;
		bool Return_Encountered = 0;

		u64 Temporary_Register_ID_Counter = 100000;
		u64 Float_Constant_Counter = 0;

		u64 Function_Counter = 0;
		u64 Label_Counter = 0;
	};
}