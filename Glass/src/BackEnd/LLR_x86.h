#pragma once

#include "BackEnd/IR.h"
#include "BackEnd/Metadata.h"

namespace Glass
{
	enum X86_ASM
	{
		X86_INVALID,

		X86_SECTION,

		X86_DEFINE_QUAD,

		X86_NAMED_OFFSET,

		X86_QUAD_DATA_ARRAY,

		X86_INST_ARRAY,

		X86_LABEL,
		X86_LABEL_REF,
		X86_RET,

		X86_CALL,

		X86_JMP,
		X86_JMPG,
		X86_JMPGE,
		X86_JMPL,
		X86_JMPLE,
		X86_JMPE,
		X86_JMPNE,

		X86_MOV,
		X86_MOVZX,
		X86_MOVSXD,
		X86_MOVQ,
		X86_LEA,

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

		X86_DE_REF,

		X86_ADDR_MUL, // '4 * edi' [rax + 4*edi] for example

		X86_DATA_STR_REF, // [hello_world_str]

		X86_CMP,

		X86_SETG,
		X86_SETGE,
		X86_SETL,
		X86_SETLE,
		X86_SETE,
		X86_SETNE,
	};

	enum X86_REG_Overlap
	{
		X86_SP, X86_BP, X86_AX, X86_CX, X86_DX, X86_BX, X86_SI, X86_DI,
		X86_R8, X86_R9, X86_R10, X86_R11, X86_R12, X86_R13, X86_R14, X86_R15,
	};

	enum X86_Register
	{
		RSP, RBP, RAX, RCX, RDX, RBX, RSI, RDI,					// 64 bit
		ESP, EBP, EAX, ECX, EDX, EBX, ESI, EDI,					// 32 bit
		SP, BP, AX, CX, DX, BX, SI, DI,							// 16 bit
		SPL, BPL, AH, AL, CH, CL, DH, DL, BH, BL, SIL, DIL,		// 8  bit

		XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7,			// 128 bit vector

		R8, R9, R10, R11, R12, R13, R14, R15,
		R8d, R9d, R10d, R11d, R12d, R13d, R14d, R15d,
		R8w, R9w, R10w, R11w, R12w, R13w, R14w, R15w,
		R8b, R9b, R10b, R11b, R12b, R13b, R14b, R15b,
	};

	enum X86_Word
	{
		X86_byte = 1,	//	8 bit

		X86_word,	//	16 bit
		X86_dword,	//	32 bit
		X86_qword,	//	64 bit

		X86_xword,	//	128 bit
		X86_yword,	//	256 bit
		X86_zword,	//	512 bit
	};

	enum RegisterUsage
	{
		REG_I8,
		REG_I16,
		REG_I32,
		REG_I64,
	};

	struct X86_Inst;

	struct X86_Label_Inst
	{
		const char* name;
	};

	struct X86_Call_Inst
	{
		X86_Inst* What;
	};

	struct X86_Jmp_Inst
	{
		X86_Inst* Where;
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

	struct X86_Data_Str_Ref_Inst
	{
		u64 string_id;
	};

	enum X86_Constant_Offset_Type {
		X86_CONSTANT_SUB,
		X86_CONSTANT_ADD,
	};

	struct X86_Constant_Offset
	{
		X86_Inst* offset;
		X86_Inst* from;
		X86_Constant_Offset_Type offset_type;
		X86_Word	size;
	};

	struct X86_Constant_Binop
	{
		X86_Inst* a;
		X86_Inst* b;
	};

	struct X86_Mov_Inst
	{
		X86_Inst* destination;
		X86_Inst* source;
	};

	struct X86_Lea_Inst
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

	struct X86_Cmp_Inst
	{
		X86_Inst* a;
		X86_Inst* b;
	};

	struct X86_Cond_Set_Inst
	{
		X86_Inst* destination;
	};

	struct X86_DeReference_Inst
	{
		X86_Inst* what;
		X86_Word size;
	};

	enum Section_Type : u8 {
		SEC_Code, SEC_Data
	};

	enum Section_Flags_Enum : u8 {
		SEC_Readable = BIT(1),
		SEC_Writable = BIT(2),
		SEC_Executable = BIT(3),
	};

	typedef u8 Section_Flags;

	struct X86_Section_Inst
	{
		Section_Flags flags;
		Section_Type type;
		const char* name;
	};

	struct X86_Named_Offset
	{
		X86_Inst* value;
		const char* name;
	};

	struct X86_Data_Array
	{
		void* data;
		u64  byte_size;
	};

	struct X86_Inst_Array_Inst
	{
		X86_Inst** data;
		u64		   count;
	};

	struct X86_Inst
	{
		X86_ASM type;

		union
		{
			X86_Named_Offset			named_offset;
			X86_Data_Array				data_array;
			X86_Inst_Array_Inst			inst_array; // lay any type of insturction in this format string, 0, 0x505 ...

			X86_Section_Inst			section;

			X86_Label_Inst				label;

			X86_Reg_Name_Inst			reg_name;
			X86_Reg_Allocation_Inst		reg_alloc;
			X86_Call_Inst				call;
			X86_Jmp_Inst				jmp;

			X86_Constant_Inst			constant;
			X86_Constant_Offset			constant_offset;
			X86_Data_Str_Ref_Inst		data_str_ref;

			X86_Mov_Inst				move;
			X86_Lea_Inst				lea;

			X86_Push_Inst				push;
			X86_Pop_Inst				pop;

			X86_BinOp_Inst				bin_op;
			X86_Cmp_Inst				cmp;
			X86_Cond_Set_Inst			cond_set;

			X86_Constant_Binop			const_binop;

			X86_DeReference_Inst		de_ref;
		} as;

		const char* comment = nullptr;
	};

	struct RegisterFreeList {
		u64 free_list[8];
		u64 count;
	};

	struct X86_BackEnd_Data
	{
		u64 CurrentFunction_StackFrameSize = 0;
		u64 CurrentFunction_CallStackSize = 0;
		u64 CurrentFunction_CallStackPointer = 0;

		u64 CurrentFunction_InputCallStackPointer = 0;
		u64 CurrentFunction_InputCallStackOffset = 0; // this will set too 8 if we do push rbp at function prologue

		std::unordered_map<u64, X86_Inst*> IR_RegisterValues;
		std::unordered_map<u64, X86_Inst*> IR_FunctionLabels;
		std::unordered_map<u64, RegisterFreeList> IR_RegisterFreeLists;

		std::unordered_map<u64, TypeStorage*> IR_RegisterTypes;
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

		void AssembleTypeInfoTable(std::string& stream);

		void AssembleData(IRData* inst, std::vector<X86_Inst*>& stream);
		void AssembleTypeOf(IRTypeOf* inst, std::vector<X86_Inst*>& stream);

		void AssembleDataValue(IRDataValue* inst, std::vector<X86_Inst*>& stream);

		void AssembleConstValue(IRCONSTValue* inst, std::vector<X86_Inst*>& stream);

		void AssembleFunction(IRFunction* inst, std::vector<X86_Inst*>& stream);
		void AssembleAlloca(IRAlloca* inst, std::vector<X86_Inst*>& stream);

		void AssembleStore(IRStore* inst, std::vector<X86_Inst*>& stream);
		void AssembleLoad(IRLoad* inst, std::vector<X86_Inst*>& stream);

		void AssembleMemberAccess(IRMemberAccess* inst, std::vector<X86_Inst*>& stream);
		void AssembleArrayAccess(IRArrayAccess* array_access, std::vector<X86_Inst*>& stream);

		void AssembleArgument(IRArgumentAllocation* inst, std::vector<X86_Inst*>& stream);

		void AssembleCall(IRFunctionCall* inst, std::vector<X86_Inst*>& stream);
		void AssembleReturn(IRReturn* inst, std::vector<X86_Inst*>& stream);

		void AssembleBinOp(IRBinOp* inst, std::vector<X86_Inst*>& stream);

		void AssembleLogicalOp(IRBinOp* inst, std::vector<X86_Inst*>& stream);

		void AssembleIf(IRIf* ir_if, std::vector<X86_Inst*>& stream);
		void AssembleWhile(IRWhile* ir_while, std::vector<X86_Inst*>& stream);

		void AssembleLexicalBlock(IRLexBlock* lexical_block, std::vector<X86_Inst*>& stream);

		void AssembleIRRegister(IRRegister* inst, std::vector<X86_Inst*>& stream);
		void AssembleIRRegisterValue(IRRegisterValue* register_value, std::vector<X86_Inst*>& stream);

		TypeStorage* GetIRNodeType(IRInstruction* inst);

		std::string Print(const std::vector<X86_Inst*>& assm);
		void Print(X86_Inst inst, std::string& stream, std::string& comments);

		std::string MangleName(const std::string& name, TSFunc* signature);

		std::string RegisterToString(X86_Register reg);

		void Make_MemCpy(u64 source_register_id, u64 destination_register_id, std::vector<X86_Inst*>& stream, TypeStorage* type);
		void Make_LocalStack_MemCpy(X86_Inst* source_stack_offset, X86_Inst* destination_stack_offset, std::vector<X86_Inst*>& stream, TypeStorage* type);

		X86_Inst* Make_Move(X86_Inst* source, X86_Inst* destination, std::vector<X86_Inst*>& intermediate_stream, TypeStorage* type, const char* comment = nullptr);
		X86_Inst* Make_Move(X86_Inst* source, X86_Inst* destination, std::vector<X86_Inst*>& intermediate_stream, u64 size, const char* comment = nullptr);

		void Make_LEA(X86_Inst* source, X86_Inst* destination, std::vector<X86_Inst*>& stream);

		X86_Inst* Register_Zext(X86_Inst* reg, u64 size, std::vector<X86_Inst*>& stream);
		X86_Inst* Register_Sext(X86_Inst* reg, u64 size, std::vector<X86_Inst*>& stream, bool free_register_after_use = true);

		X86_Inst* Make_Register(X86_Register register_type);
		X86_Inst* Make_Constant(i64 integer);

		X86_Inst* GetIRRegister(u64 id, bool free_allocated_registers = true);
		TypeStorage* GetIRRegisterType(u64 id);

		RegisterUsage RegisterUsageBySize(TypeStorage* type);
		RegisterUsage RegisterUsageBySize(u64 type_size);

		X86_Word InWords(TypeStorage* type);
		X86_Word InWords(u64 type_size);

		u64 RegiserSize(X86_Register reg);

		std::map<u32, X86_Inst*> RegisterAllocationIDs;
		std::map<X86_REG_Overlap, X86_Inst*> RegisterAllocations;
		std::map<X86_REG_Overlap, TypeStorage*> RegisterAllocationTypes;
		std::map<X86_REG_Overlap, bool> RegisterOccupations;

		void Free_Register(u32 id);
		void Free_All_Register();

		X86_Inst* Allocate_Register(RegisterUsage usage, u32 id, std::vector<X86_Inst*>& spillage_stream, bool free_register_after_use = true);
		X86_Inst* Allocate_Specific_Register(X86_Register reg, u32 id, std::vector<X86_Inst*>& spillage_stream);

		bool AreEqual(X86_Inst* a, X86_Inst* b);

		X86_Inst* AllocateStack(TypeStorage* type);
		X86_Inst* AllocateStack(u64 allocation_size);

		X86_Inst* Allocate_CallStack(u64 allocation_size);
		void Reset_CallStackPointer();
		void Reset_CallStackSize();

		u32 GetRegisterID();

		enum GetArgument_Location_Dir
		{
			ARG_DIR_IN = 0,
			ARG_DIR_OUT = 1,
		};

		enum ArgumentLocationType {
			ARG_LOC_PTR_IN_STACK,
			ARG_LOC_REGISTER,
			ARG_LOC_REGISTER_PTR,
		};

		struct ArgumentLocationInfo
		{
			ArgumentLocationType Type;
		};

		ArgumentLocationInfo GetArgumentLocationInfo(TypeStorage* type, u32 index, GetArgument_Location_Dir direction);

		X86_Inst* GetArgumentLocation(TypeStorage* type, u32 index, std::vector<X86_Inst*>& spillage_stream, GetArgument_Location_Dir direction);
		X86_Inst* GetReturnLocation(TypeStorage* type, std::vector<X86_Inst*>& spillage_stream);

		u32 RegisterIDCounter = 0;

		std::vector<std::pair<u64, std::vector<char>>> m_DataStrings;
		std::vector<std::pair<u64, std::vector<char>>> m_TypeInfoStrings;
		std::vector<std::string> m_Externals; // currently name is the linkage name

		IRTranslationUnit* m_TranslationUnit;
		MetaData* m_Metadata;

		FunctionMetadata* m_CurrentFunction;

		bool VariableRegisterPromotion = false;
		bool UselessMoveElimination = true;

		bool CalledVariadicFunction = false;

		bool IsRegisterValue = false;
		X86_Inst* RegisterValue = nullptr;

		void SetRegisterValue(X86_Inst* register_value) {
			GS_CORE_ASSERT(!RegisterValue);
			RegisterValue = register_value;
		}

		void SetRegisterFreeList(u64 ir_register_id, const RegisterFreeList& free_list) {
			m_Data.IR_RegisterFreeLists[ir_register_id] = free_list;
		}

		X86_Inst* CurrentReturnTarget = nullptr;
		X86_Inst* ReturnJmpTarget = nullptr;

		X86_Inst* MemCpy = nullptr;

		u64 FunctionCounter = 0;
		u64 LabelCounter = 0;

		u64 TypeInfoStringCounter = 0;

		u64 GetStringID() {
			TypeInfoStringCounter++;
			return TypeInfoStringCounter;
		}

		IRRegister* CurrentRegister = nullptr;

		const char* GetContLabelName() {
			auto label_name = ASMA(fmt::format("cont_{}_{}", FunctionCounter, LabelCounter))->c_str();
			LabelCounter++;
			return label_name;
		}

		const char* GetLoopLabelName() {
			auto label_name = ASMA(fmt::format("loop_{}_{}", FunctionCounter, LabelCounter))->c_str();
			LabelCounter++;
			return label_name;
		}

		std::vector<X86_Inst*> CurrentFunctionArgumentAllocations;
		X86_Inst* CurrentFunctionReturnAllocation = nullptr;

		X86_BackEnd_Data m_Data;
	};
}