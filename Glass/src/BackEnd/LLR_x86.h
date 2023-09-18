#pragma once

#include "BackEnd/IR.h"
#include "BackEnd/Metadata.h"

namespace Glass
{
	enum X86_ASM
	{
		X86_LABEL,
		X86_RET,

		X86_MOV,
		X86_MOVQ,

		X86_PUSH,
		X86_PUSHQ,

		X86_POP,
		X86_POPQ,

		X86_ISUB,
		X86_IADD,

		X86_REG_NAME,

		X86_CONSTANT,

		X86_CONSTANT_OFFSET, // [rsp, -16] for example
	};

	enum X86_Register
	{
		RSP, RBP, RAX, RCX, RDX, RBX, RSI, RDI,					// 64 bit
		ESP, EBP, EAX, ECX, EDX, EBX, ESI, EDI,					// 32 bit
		SP, BP, AX, CX, DX, BX, SI, DI,							// 16 bit
		SPL, BPL, AH, AL, CH, CL, DH, DL, BH, BL, SIL, DIL,		// 8  bit

		XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7,			// 128 bit vector
	};

	enum X86_Word
	{
		word,
		dword,
		qword,
	};

	struct X86_Inst;

	struct X86_Label_Inst
	{
		const char* name;
	};

	struct X86_Reg_Name_Inst
	{
		X86_Register Register;
	};

	struct X86_Constant_Inst
	{
		u8 bytes[8];
	};

	struct X86_Constant_Offset
	{
		X86_Inst* offset;
		X86_Inst* from;
		X86_Word	size;
	};

	struct X86_Mov_Inst
	{
		X86_Inst* destination;
		X86_Inst* source;
	};

	struct X86_Push_Inst
	{
		X86_Inst* source;
	};

	struct X86_Pop_Inst
	{
		X86_Inst* destination;
	};

	struct X86_BinOp_Inst
	{
		X86_Inst* destination;
		X86_Inst* value;
	};

	struct X86_Inst
	{
		X86_ASM type;

		union
		{
			X86_Label_Inst			label;
			X86_Reg_Name_Inst		reg_name;
			X86_Constant_Inst		constant;
			X86_Constant_Offset		constant_offset;

			X86_Mov_Inst			move;

			X86_Push_Inst			push;
			X86_Pop_Inst			pop;

			X86_BinOp_Inst			bin_op;
		} as;
	};

	class X86_BackEnd
	{
	public:
		X86_BackEnd(IRTranslationUnit* translation_unit, MetaData* metadata);

		std::vector<X86_Inst*> Assemble();
		void Assemble(IRInstruction* inst, std::vector<X86_Inst*>& stream);

		std::string Print(const std::vector<X86_Inst>& assm);
		void Print(X86_Inst inst, std::string& stream, u32& indentation);

		std::string RegisterToString(X86_Register reg);

		X86_Inst* Make_Register(X86_Register register_type);
		X86_Inst* Make_Constant(i64 integer);

		std::map<u32, X86_Register> RegisterAllocationIDs;
		std::map<X86_Register, bool> RegisterOccupations;

		X86_Register Get_Register(u32 register_id);

		IRTranslationUnit* m_TranslationUnit;
		MetaData* m_Metadata;
	};
}