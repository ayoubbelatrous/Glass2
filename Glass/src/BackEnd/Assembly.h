#pragma once

namespace Glass {

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

		I_CvtSI2SS,
		I_CvtSI2SD,

		I_CvtSS2SI,
		I_CvtSD2SI,

		I_CvtSS2SD,
		I_CvtSD2SS,

		I_UCOMISS,
		I_UCOMISD,

		I_CBW,
		I_CWD,
		I_CDQ,
		I_CQO,

		I_Cmp,

		I_Setne,
		I_Sete,

		I_Setl,
		I_Setg,

		I_Seta,
		I_Setb,

		I_Setle,
		I_Setge,

		I_Je,
		I_Jne,

		I_Jg,
		I_Jl,

		I_Ja,
		I_Jb,

		I_Jle,
		I_Jge,

		I_Jbe,
		I_Jae,

		I_Jmp,

		I_And,
		I_Or,

		I_Call,

		I_Label,
	};

	enum X86_Register {
		None,

		RIP,
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
		asm_none = 0,
		asm_byte = 1,
		asm_word = 2,
		asm_dword = 3,
		asm_qword = 4,
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

	struct Assembly_External_Symbol {
		std::string ExternalName;
		std::string Name;
	};

	struct Assembly_Dynamic_Library {
		std::string Name;
		std::string Path;
	};

	struct Assembly_Import {
		u64 library_idx = -1;
		std::string Name;
	};

	enum class Assembly_Global_Linkage {
		Private,
		Import,
		External,
	};

	enum class Assembly_Global_Initializer_Type {
		Zero_Initilizer,
		Bytes_Initilizer,
		DoubleWords_Initilizer,
	};

	struct Assembly_Global_Initializer {
		Assembly_Global_Initializer_Type Type;
		std::vector<u8> Initializer_Data;
	};

	struct Assembly_Global {
		std::string Name;
		u64 Allocation_Size = 0;
		Assembly_Global_Linkage Linkage;
		Assembly_Global_Initializer Initializer;
	};

	struct Assembly_TypeInfo_Table_Entry {
		Assembly_Operand* elements[8];
		u64 count = 0;
	};

	struct Assembly_TypeInfo_Table {
		std::string External_Variable_Name;
		std::string External_Length_Name;
		std::string External_Members_Array_Name;
		std::string External_Enum_Members_Array_Name;
		std::string External_Func_Type_Parameters_Array_Name;
		std::vector<Assembly_TypeInfo_Table_Entry> Func_Type_Parameters_Array;
		std::vector<Assembly_TypeInfo_Table_Entry> Enum_Members_Array;
		std::vector<Assembly_TypeInfo_Table_Entry> Members_Array;
		std::vector<Assembly_TypeInfo_Table_Entry> Entries;
	};

	enum class Assembler_Output_Mode {
		COFF_Object,
		PE_Executable,
		PE_Dll,
	};

	struct Assembly_File {

		Assembler_Output_Mode output_mode;

		std::vector<Assembly_External_Symbol> externals;

		std::vector<Assembly_Function> functions;
		std::vector<Assembly_Float_Constant> floats;
		std::vector<Assembly_String_Constant> strings;

		std::vector<Assembly_Dynamic_Library> libraries;
		std::vector<Assembly_Import> imports;

		std::vector<Assembly_Global> globals;
		Assembly_TypeInfo_Table type_info_table;
	};

}