#pragma once

#include "BackEnd/IR.h"
#include "BackEnd/Metadata.h"

#include "Assembly.h"
#include "Intel_Syntax_Printer.h"

namespace Glass
{
	struct DIE_CU {
		u16 FileName_Str_Idx;
		u16 Directory_Str_Idx;
		u16 CompilerName_Str_Idx;
	};

	struct DIE_File {
		std::string FileName;
		std::string Directory;
	};

	struct DIE_Procedure {
		u16 Name_Str_Idx;
		TSFunc* Signature = nullptr;

		DBGSourceLoc Definition_Src_Loc;

		DBGSourceLoc Begin_Src_Loc;
		DBGSourceLoc End_Src_Loc;
		u32 End_Prologue_Inst_Idx = -1;
		u32 Begin_Epilogue_Inst_Idx = -1;

		u16 File_Idx = -1;
		u16 Function_Idx = -1;
		std::vector<DBGSourceLoc> Inst_Line_Table;
		i32	Last_Inst = 0;
	};

	struct Debug_Info {
		DIE_CU Compile_Unit;
		std::vector<DIE_File> Files;
		std::vector<DIE_Procedure> Procedures;
		std::vector<std::string> String_Table;
		u16 Current_File_Idx = -1;
		u16 Current_Procedure_Idx = -1;
	};

	struct Debug_Info_Builder {
		static void Build_Compile_Unit(Debug_Info& di, const std::string& name, const std::string& directory, const std::string& compiler_name);
		static u16 Build_File(Debug_Info& di, const std::string& name, const std::string& directory);
		static u16 Build_Procedure(Debug_Info& di, u16 function_idx, const std::string& name, DBGSourceLoc def_loc, DBGSourceLoc begin_loc, DBGSourceLoc end_loc);
		static u16 Build_String(Debug_Info& di, const std::string& str);
		static void Set_Location(Debug_Info& di, DBGSourceLoc location, i32 inst_idx);
		static void Finalize_Procedure(Debug_Info& di, i32 inst_idx);
	};

	struct FASM_Printer {

		FASM_Printer(Assembly_File* assembly);

		std::string Print();
		void PrintCode(std::stringstream& stream);

		Assembly_File* Assembly = nullptr;

		Intel_Syntax_Printer intel_syntax_printer;
	};

	struct Clang_Assembler_Printer {

		Clang_Assembler_Printer(Assembly_File* assembly);
		Clang_Assembler_Printer(Assembly_File* assembly, Debug_Info debug_info);

		std::string Print();
		void PrintCode(std::stringstream& stream);
		void PrintDebugInfo(std::stringstream& stream);

		Assembly_File* Assembly = nullptr;

		bool Build_Debug_Info = false;
		Debug_Info Di;

		Intel_Syntax_Printer intel_syntax_printer;
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

	enum X86_Register_Family {

		F_None,

		F_BP,
		F_SP,

		F_A,

		F_R10,
		F_R11,
		F_R12,
		F_R13,
		F_R14,
		F_R15,

		F_B,

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
		Immediate_Value,	// 50, 60 etc
		Register_Value,		// rax or 16
		Memory_Value,		// qword [rax + 16] or qword [rbp - 16] or dword [flt_60]
		Stack_Address,		// rbp - 4
		Pointer_Address,	// rbx + 4 rbx
	};

	struct X86_BackEnd_Data
	{
		std::unordered_map<u64, Assembly_Operand*> IR_RegisterValues;
		std::unordered_map<u64, TypeStorage*> IR_RegisterTypes;
		std::unordered_map<u64, u64> IR_RegisterLifetimes;
		std::unordered_map<u64, Register_Value_Type> IR_RegisterValueTypes;

		std::unordered_map<u64, Assembly_Function*> Functions;
		std::unordered_map<u64, std::map<TSFunc*, Assembly_Function*>> FunctionOverloads;

		std::unordered_map<std::string, u64> Library_Indices;

		u64 Stack_Size = 0;
		u64 Call_Stack_Size = 0;
		u64 Call_Stack_Pointer = 0;
	};

	class X86_BackEnd
	{
	public:
		X86_BackEnd(IRTranslationUnit* translation_unit, MetaData* metadata, bool use_linker = false, CodeGen_Assembler assembler = CodeGen_Assembler::Fasm);

		void Init();

		std::string Mangle_Name(std::string name, TypeStorage* type);

		void AssembleForeignLibraries();
		void AssembleForeignImport(const FunctionMetadata* function);
		void AssembleExternalFunction(const FunctionMetadata* function);
		void AssembleExternals();
		void AssembleTypeInfoTable();
		void Assemble();

		void AssembleFasm();
		void AssembleClang_Asm();

		void AssembleInstruction(IRInstruction* instruction);
		void AssembleFunctionSymbol(IRFunction* ir_function);
		void AssembleFunction(IRFunction* ir_function);

		void AssembleFunctionCall(IRFunctionCall* ir_function);

		void AssembleRegister(IRRegister* ir_register);

		void AssembleArgument(IRArgumentAllocation* ir_argument);

		void AssembleAlloca(IRAlloca* ir_alloca);

		void AssembleMemberAccess(IRMemberAccess* ir_member_access);
		void AssembleArrayAccess(IRArrayAccess* ir_array_access);

		void Spill_All_Scratch();
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
		void AssembleSRem(IRSREM* ir_srem);

		void AssembleAnd(IRAnd* ir_and);
		void AssembleOr(IROr* ir_or);
		void AssembleEqual(IREQ* ir_eq);
		void AssembleNotEqual(IRNOTEQ* ir_not_eq);

		void AssembleLesser(IRLesser* ir_lesser);
		void AssembleGreater(IRGreater* ir_greater);

		void AssembleBitAnd(IRBitAnd* ir_bit_and);
		void AssembleBitOr(IRBitOr* ir_bit_or);

		void AssemblePointerCast(IRPointerCast* ir_pointer_cast);
		void AssembleInt2PCast(IRInt2PtrCast* ir_int_2_ptr);
		void AssemblePtr2IntCast(IRPtr2IntCast* ir_ptr_2_int);
		void AssembleSextCast(IRSExtCast* ir_sext);
		void AssembleZextCast(IRZExtCast* ir_zext);
		void AssembleIntTruncCast(IRIntTrunc* ir_int_trunc);
		void AssembleInt2FPCast(IRInt2FP* ir_int_2_fp);
		void AssembleFP2IntCast(IRFP2Int* ir_fp_2_int);
		void AssembleFPExtCast(IRFPExt* ir_fp_ext);
		void AssembleFPTruncCast(IRFPTrunc* ir_fp_trunc);

		void AssembleFuncRef(IRFuncRef* ir_func_ref);
		void AssembleCallFuncRef(IRCallFuncRef* ir_call_func_ref);

		void AssembleGlobalDeclare(IRGlobalDecl* ir_global);
		void AssembleGlobalAddress(IRGlobalAddress* ir_global_addr);

		void AssembleTypeValue(IRTypeValue* ir_type_value);
		void AssembleTypeInfo(IRTypeInfo* ir_type_info);

		void AssembleBreak(IRBreak* ir_break);
		void AssembleNullPtr(IRNullPtr* ir_null_ptr);
		void AssembleIntrinsic_MemSet(IRIntrinsicMemSet* ir_memset);

		void AssembleAnyArray(IRAnyArray* ir_any_array);

		void AssembleString_Initializer(IRStringInitializer* ir_string_initializer);

		void AssembleReturn(IRReturn* ir_return);

		void AssembleConstValue(IRCONSTValue* ir_constant);

		std::unordered_map<u64, Assembly_Operand*> data_values;

		void AssembleData(IRData* ir_data);
		void AssembleDataValue(IRDataValue* ir_data_value);

		TypeStorage* GetIRNodeType(IRInstruction* ir);

		X86_BackEnd_Data m_Data;
		IRTranslationUnit* m_TranslationUnit = nullptr;
		MetaData* m_Metadata = nullptr;

		std::vector<Assembly_Dynamic_Library> Dynamic_Libraries;
		std::vector<Assembly_Import> Imports;
		std::vector<Assembly_External_Symbol> Externals;
		std::vector<Assembly_String_Constant> Strings;
		std::vector<Assembly_Function*> Functions;
		std::vector<Assembly_Instruction> Code;
		std::vector<Assembly_Float_Constant> Floats;
		std::vector<Assembly_Global> Globals;
		Assembly_TypeInfo_Table Program_Assembly_TypeInfo_Table;

		std::string GetLabelName();

		void Call_Memcpy();

		Assembly_Operand* Stack_Alloc(TypeStorage* type);
		Assembly_Operand* Stack_Alloc(u64 size);
		Assembly_Operand* Alloc_Call_StackTop(TypeStorage* type);
		Assembly_Operand* GetReturnRegister(TypeStorage* type);

		Assembly_Instruction MoveBasedOnType(TypeStorage* type, Assembly_Operand* op1, Assembly_Operand* op2, const char* comment = nullptr);

		u64 CurrentRegister = 0;
		IRRegister* CurrentIrRegister = nullptr;

		void SetRegisterValue(Assembly_Operand* register_value, Register_Value_Type value_type);
		void SetRegisterValue(Assembly_Operand* register_value, u64 register_id);
		void SetRegisterValue(Assembly_Operand* register_value, u64 register_id, Register_Value_Type value_type);
		u64 CreateTempRegister(Assembly_Operand* register_value);

		std::tuple<Assembly_Operand*, u64> Allocate_Temp_Physical_Register(TypeStorage* type);
		std::tuple<Assembly_Operand*, u64> Allocate_Temp_Physical_Register(TypeStorage* type, X86_Register physical_register);

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
		Assembly_Operand* Create_String_Constant(const std::string& data);

		std::set<u64> Current_Function_Used_Registers;

		void Push_Used_Register(X86_Register physical_register);

		bool Is_Volatile(X86_Register physical_register);

		bool Are_Equal(Assembly_Operand* operand1, Assembly_Operand* operand2);

		Register_Allocation_Data Register_Allocator_Data;

		Assembly_Operand* Return_Label = nullptr;

		Assembly_Operand* TypeInfo_Table_Label = nullptr;

		Assembly_Operand* Return_Storage_Location = nullptr;
		bool Return_Encountered = false;
		bool Break_Encountered = false;

		u64 Temporary_Register_ID_Counter = 100000;
		u64 Float_Constant_Counter = 0;

		u64 Function_Counter = 0;
		u64 Label_Counter = 0;

		u64 String_Id_Counter = 64000;

		u64 Current_Register_Lifetime = 0;

		Assembly_Operand* Current_Skip_Target = nullptr;
		std::stack<Assembly_Operand* > Skip_Target_Stack;

		bool Use_Linker = true;
		bool Rip_Relative = true;

		CodeGen_Assembler assembler = CodeGen_Assembler::Fasm;

		bool Debug = true;
		Debug_Info di;
	};
}