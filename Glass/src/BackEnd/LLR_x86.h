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
	};

	enum X86_Register {
		RBP,
		RSP,
		RAX,
		RBX,
	};

	struct Assembly_Operand;

	enum Assembly_Operand_Type {
		Op_Register,
		Op_Constant_Integer,
	};

	struct Assembly_Operand_Register {
		X86_Register Register;
	};

	struct Assembly_Operand_Constant_Integer {
		i64 integer = 0;
	};

	struct Assembly_Operand_Dereference {
		Assembly_Operand* operand;
	};

	struct Assembly_Operand {
		Assembly_Operand_Type type;
		union {
			Assembly_Operand_Register reg;
			Assembly_Operand_Dereference de_reference;
			Assembly_Operand_Constant_Integer constant_integer;
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
	};

	struct FASM_Printer {

		FASM_Printer(Assembly_File* assembly);

		std::string Print();
		void PrintOperand(const Assembly_Operand* operand, std::stringstream& stream);
		void PrintInstruction(const Assembly_Instruction& instruction, std::stringstream& stream);
		void PrintCode(std::stringstream& stream);

		Assembly_File* Assembly = nullptr;
	};

	struct Builder
	{
		static Assembly_Instruction Ret();

		static Assembly_Instruction Push(Assembly_Operand* operand);
		static Assembly_Instruction Pop(Assembly_Operand* operand);
		static Assembly_Instruction Add(Assembly_Operand* operand1, Assembly_Operand* operand2);
		static Assembly_Instruction Sub(Assembly_Operand* operand1, Assembly_Operand* operand2);

		static Assembly_Operand* Register(X86_Register reg);
		static Assembly_Operand* Constant_Integer(i64 integer);
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

		TypeStorage* GetIRNodeType(IRInstruction* ir);

		X86_BackEnd_Data m_Data;
		IRTranslationUnit* m_TranslationUnit = nullptr;
		MetaData* m_Metadata = nullptr;

		std::vector<Assembly_External_Symbol> Externals;
		std::vector<Assembly_String> Strings;
		std::vector<Assembly_Function> Functions;
		std::vector<Assembly_Instruction> Code;
	};
}