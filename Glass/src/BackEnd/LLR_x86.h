#pragma once

#include "BackEnd/IR.h"
#include "BackEnd/Metadata.h"

namespace Glass
{
	struct Assembly_External_Symbol {
		std::string ExternalName;
		std::string Name;
	};

	struct Assembly_String {
		std::string String;
	};

	enum Assembly_Op_Code {
		I_Ret,

		I_Push,
		I_Pop,

		I_Add,
		I_Sub,

		I_Mov,
	};

	enum X86_Register {
		RBP,
		RSP,

		AL,
		BL,

		AX,
		BX,

		EAX,
		EBX,

		RAX,
		RBX,
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
	};

	struct Assembly_Function {
		std::string Name;
		Assembly_Operand* Label = nullptr;
		std::vector<Assembly_Instruction> Code;
	};

	struct Assembly_File {
		std::vector<Assembly_External_Symbol> externals;
		std::vector<Assembly_String> strings;
		std::vector<Assembly_Function> functions;
	};

	struct X86_BackEnd_Data
	{
		std::unordered_map<u64, Assembly_Operand*> IR_RegisterValues;
		std::unordered_map<u64, TypeStorage*> IR_RegisterTypes;
		std::unordered_map<u64, Assembly_Function*> Functions;

		u64 Stack_Size = 0;
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

		static Assembly_Instruction Push(Assembly_Operand* operand);
		static Assembly_Instruction Pop(Assembly_Operand* operand);
		static Assembly_Instruction Add(Assembly_Operand* operand1, Assembly_Operand* operand2);
		static Assembly_Instruction Sub(Assembly_Operand* operand1, Assembly_Operand* operand2);

		static Assembly_Instruction Mov(Assembly_Operand* operand1, Assembly_Operand* operand2);

		static Assembly_Operand* OpAdd(Assembly_Operand* operand1, Assembly_Operand* operand2);
		static Assembly_Operand* OpSub(Assembly_Operand* operand1, Assembly_Operand* operand2);
		static Assembly_Operand* OpMul(Assembly_Operand* operand1, Assembly_Operand* operand2);
		static Assembly_Operand* OpDiv(Assembly_Operand* operand1, Assembly_Operand* operand2);

		static Assembly_Operand* Register(X86_Register reg);
		static Assembly_Operand* Constant_Integer(i64 integer);
		static Assembly_Operand* De_Reference(Assembly_Operand* operand1, TypeStorage* type = nullptr);
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

		void AssembleRegister(IRRegister* ir_register);

		void AssembleReturn(IRReturn* ir_return);

		void AssembleConstValue(IRCONSTValue* ir_constant);

		TypeStorage* GetIRNodeType(IRInstruction* ir);

		X86_BackEnd_Data m_Data;
		IRTranslationUnit* m_TranslationUnit = nullptr;
		MetaData* m_Metadata = nullptr;

		std::vector<Assembly_External_Symbol> Externals;
		std::vector<Assembly_String> Strings;
		std::vector<Assembly_Function> Functions;
		std::vector<Assembly_Instruction> Code;

		Assembly_Operand* Stack_Alloc(TypeStorage* type);
		Assembly_Operand* GetReturnRegister(TypeStorage* type);

		u64 CurrentRegister = 0;

		void SetRegisterValue(Assembly_Operand* register_value);
		Assembly_Operand* GetRegisterValue(IRRegisterValue* ir_register);

		Assembly_Operand* Return_Storage_Location = nullptr;
		u64 Return_Counter = 0;
	};
}