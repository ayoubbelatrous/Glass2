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

		X86_IMUL,
		X86_IDIV,

		X86_REG_NAME,

		X86_REG_ALLOC,

		X86_CONSTANT,

		X86_CONSTANT_OFFSET, // [rsp + 16] for example
	};

	enum X86_REG_Overlap
	{
		X86_SP, X86_BP, X86_AX, X86_CX, X86_DX, X86_BX, X86_SI, X86_DI
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

	enum RegisterUsage
	{
		REG_I32,
		REG_I64,
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

	struct X86_Reg_Allocation_Inst
	{
		X86_Register Register;
		u32 register_allocation_id;
		bool free_after_use;
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
			X86_Label_Inst				label;

			X86_Reg_Name_Inst			reg_name;
			X86_Reg_Allocation_Inst		reg_alloc;

			X86_Constant_Inst			constant;
			X86_Constant_Offset			constant_offset;

			X86_Mov_Inst				move;

			X86_Push_Inst				push;
			X86_Pop_Inst				pop;

			X86_BinOp_Inst				bin_op;
		} as;
	};

	struct X86_BackEnd_Data
	{
		u64 CurrentFunction_StackFrameSize = 0;
		std::map<u64, X86_Inst*> IR_RegisterValues;
	};

	struct X86_Register_Allocation {
		X86_Register Register;
		bool free_after_use = false;
	};

	class X86_BackEnd
	{
	public:
		X86_BackEnd(IRTranslationUnit* translation_unit, MetaData* metadata);

		std::vector<X86_Inst*> Assemble();
		void Assemble(IRInstruction* inst, std::vector<X86_Inst*>& stream);

		void AssembleConstValue(IRCONSTValue* inst, std::vector<X86_Inst*>& stream);

		void AssembleFunction(IRFunction* inst, std::vector<X86_Inst*>& stream);
		void AssembleAlloca(IRAlloca* inst, std::vector<X86_Inst*>& stream);

		void AssembleStore(IRStore* inst, std::vector<X86_Inst*>& stream);
		void AssembleLoad(IRLoad* inst, std::vector<X86_Inst*>& stream);

		void AssembleBinOp(IRBinOp* inst, std::vector<X86_Inst*>& stream);

		void AssembleIRRegister(IRSSA* inst, std::vector<X86_Inst*>& stream);

		std::string Print(const std::vector<X86_Inst*>& assm);
		void Print(X86_Inst inst, std::string& stream);

		std::string MangleName(const std::string& name, TSFunc* signature);

		std::string RegisterToString(X86_Register reg);

		X86_Inst* Make_Register(X86_Register register_type);
		X86_Inst* Make_Constant(i64 integer);

		X86_Inst* GetIRRegister(u64 id);

		std::map<u32, X86_Inst*> RegisterAllocationIDs;
		std::map<X86_REG_Overlap, bool> RegisterOccupations;

		void Free_Register(u32 id);
		X86_Inst* Allocate_Register(RegisterUsage usage, u32 id);

		bool AreEqual(X86_Inst* a, X86_Inst* b);

		u32 GetRegisterID();

		X86_Inst* GetArgumentLocation(TypeStorage* type);
		X86_Inst* GetReturnLocation(TypeStorage* type);

		u32 RegisterIDCounter = 0;

		IRTranslationUnit* m_TranslationUnit;
		MetaData* m_Metadata;

		bool VariableRegisterPromotion = false;
		bool UselessMoveElimination = true;

		X86_BackEnd_Data m_Data;
	};
}