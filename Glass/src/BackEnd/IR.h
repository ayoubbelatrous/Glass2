#pragma once

#include "BackEnd/Type.h"
#include "Base/Hash.h"
#include "FrontEnd/Token.h"

namespace Glass
{

	struct TypeStorage;

	enum class IRType : u64
	{
		IR_void,

		IR_int,
		IR_float,

		IR_i8,
		IR_i16,
		IR_i32,
		IR_i64,

		IR_u8,
		IR_u16,
		IR_u32,
		IR_u64,

		IR_f32,
		IR_f64,

		IR_bool,

		IR_type,

		IR_any,

		IR_array,

		IR_typeinfo_flags,

		IR_typeinfo_member,
		IR_typeinfo_struct,

		IR_typeinfo,
	};

	enum IRCType : u64
	{
		IR_void,

		IR_int,
		IR_float,

		IR_i8,
		IR_i16,
		IR_i32,
		IR_i64,

		IR_u8,
		IR_u16,
		IR_u32,
		IR_u64,

		IR_f32,
		IR_f64,

		IR_bool,

		IR_type,

		IR_any,

		IR_array,

		IR_typeinfo_flags,

		IR_typeinfo_member,
		IR_typeinfo_struct,

		IR_typeinfo,
	};

	enum class IRNodeType
	{
		Invalid,

		SSA,
		SSAValue,

		ConstValue,

		Alloca,
		ArrayAllocate,
		Any,
		AnyArray,

		ADD,
		SUB,
		MUL,
		DIV,

		Equal,
		NotEqual,
		GreaterThan,
		LesserThan,
		BitAnd,
		BitOr,

		And,
		Or,

		If,
		While,
		Return,
		Break,

		Argument,

		Function,
		ForeignFunction,
		FuncRef,
		CallFuncRef,

		Call,
		Store,
		Load,
		Data,
		DataValue,
		Struct,
		StructMember,
		MemberAccess,
		ArrayAccess,

		Ref,
		DeRef,

		TypeOf,
		SizeOf,

		TypeInfo,

		TypeValue,

		GlobAddress,
		GlobDecl,

		PointerCast,

		Int2PtrCast,
		Ptr2IntCast,

		SExtCast,
		ZExtCast,

		IntTrunc,

		Int2FP,
		FP2Int,

		FPExt,
		FPTrunc,

		NullPtr,

		Label,
		Iterator,

		LexicalBlock,

		TranslationUnit,
		File,
	};

	struct IRInstruction {
		virtual std::string ToString() const {
			return "IR Instruction";
		}

		virtual IRNodeType GetType() const = 0;
	};

	struct IRArgumentAllocation : public IRInstruction {

		u64 ArgumentIndex = 0;
		TypeStorage* AllocationType = nullptr;

		IRArgumentAllocation() = default;
		IRArgumentAllocation(u64 argumentIndex, TypeStorage* allocation_type)
			:ArgumentIndex(argumentIndex), AllocationType(allocation_type)
		{
		}

		virtual std::string ToString() const override {
			return "ARG $";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::Argument;
		}
	};

	struct IRLexBlock : public IRInstruction {

		Token Begin;
		Token End;

		std::vector<IRInstruction*> Instructions;

		virtual std::string ToString() const {
			return "{}";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::LexicalBlock;
		}
	};

	struct IRNullPtr : public IRInstruction {

		IRNullPtr(u64 type_id, u64 indirection)
			:TypeID(type_id), Indirection(indirection)
		{}

		u64 TypeID = -1;
		u64 Indirection = -1;

		virtual std::string ToString() const {
			return "NullPtr";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::NullPtr;
		}
	};

	struct IRTypeValue : public IRInstruction {

		IRTypeValue(TypeStorage* type)
			:Type(type)
		{}

		TypeStorage* Type;

		virtual std::string ToString() const {
			return "TypeValue()";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::TypeValue;
		}
	};

	struct IRCONSTValue : public IRInstruction {
		u64 ID = 0;
		uint8_t Data[16];
		u64 Type = 0;

		virtual std::string ToString() const override {
			std::string str;
			i64 data = 0;
			memcpy(&data, &Data, sizeof(i64));

			str += std::to_string(data);
			return str;
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::ConstValue;
		}
	};

	struct IRSSAValue : public IRInstruction {
		u64 ID = 0;
		u64 RegisterID = 0;

		IRSSAValue() = default;
		IRSSAValue(u64 ssa)
			:RegisterID(ssa)
		{
		}

		virtual std::string ToString() const override {
			return 	"$" + std::to_string(RegisterID);
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::SSAValue;
		}
	};

	struct IRBinOp : public IRInstruction {
		IRSSAValue* SSA_A = nullptr;
		IRSSAValue* SSA_B = nullptr;
		u64 Type;

		virtual std::string ToString() const override {
			return "";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::ADD;
		}
	};

	struct IRADD : public IRInstruction {
		IRSSAValue* SSA_A = nullptr;
		IRSSAValue* SSA_B = nullptr;
		u64 Type;

		IRADD() = default;
		IRADD(IRSSAValue* A, IRSSAValue* B)
			:SSA_A(A), SSA_B(B)
		{
		}
		IRADD(IRSSAValue* A, IRSSAValue* B, u64 type)
			:SSA_A(A), SSA_B(B), Type(type)
		{
		}

		virtual std::string ToString() const override {
			std::string str;

			str += "ADD ";

			str += SSA_A->ToString();
			str += " : ";
			str += SSA_B->ToString();

			return str;
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::ADD;
		}
	};

	struct IRSUB : public IRInstruction {
		IRSSAValue* SSA_A = nullptr;
		IRSSAValue* SSA_B = nullptr;
		u64 Type;

		IRSUB() = default;

		IRSUB(IRSSAValue* a, IRSSAValue* b, u64 type)
			: SSA_A(a), SSA_B(b), Type(type)
		{
		}

		virtual std::string ToString() const override {
			std::string str;

			str += "SUB ";

			str += SSA_A->ToString();
			str += " : ";
			str += SSA_B->ToString();

			return str;
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::SUB;
		}
	};

	struct IRMUL : public IRInstruction {
		IRSSAValue* SSA_A = nullptr;
		IRSSAValue* SSA_B = nullptr;
		u64 Type;

		IRMUL() = default;
		IRMUL(IRSSAValue* A, IRSSAValue* B)
			:SSA_A(A), SSA_B(B)
		{
		}

		virtual std::string ToString() const override {
			std::string str;

			str += "MUL ";

			str += SSA_A->ToString();
			str += " : ";
			str += SSA_B->ToString();

			return str;
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::MUL;
		}
	};

	struct IRDIV : public IRInstruction {
		IRSSAValue* SSA_A = nullptr;
		IRSSAValue* SSA_B = nullptr;
		u64 Type;

		virtual std::string ToString() const override {
			std::string str;

			str += "DIV ";

			str += SSA_A->ToString();
			str += " : ";
			str += SSA_B->ToString();

			return str;
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::DIV;
		}
	};

	enum class RegType
	{
		None = 0,
		ExprResult,
		CallResult,
		Argument,

		VarAddress,
		VarValue,

		MemberAddress,
	};


	struct DBGSourceLoc {

		DBGSourceLoc() = default;

		DBGSourceLoc(u32 line, u32 col)
			:Line(line), Col(col)
		{}

		u32 Line = 0;
		u32 Col = 0;
	};

	struct IRSSA : public IRInstruction {
		u64 ID = 0;
		IRInstruction* Value = nullptr;

		virtual std::string ToString() const override {
			std::string str = "$ ";

			str += std::to_string(ID);
			str += " = ";
			if (Value) {
				str += Value->ToString();
			}

			return str;
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::SSA;
		}

		DBGSourceLoc DBGLoc;

		void SetDBGLoc(DBGSourceLoc dbg_loc) {
			DBGLoc = dbg_loc;
		}

		DBGSourceLoc GetDBGLoc() const {
			return DBGLoc;
		}
	};

	struct VariableMetadata;

	struct IRAlloca : public IRInstruction {
		u64 ID = 0;

		TypeStorage* Type = nullptr;

		const VariableMetadata* VarMetadata = nullptr;

		IRAlloca(TypeStorage* type)
			:Type(type)
		{}

		IRAlloca(TypeStorage* type, const VariableMetadata* var_metadata)
			:Type(type), VarMetadata(var_metadata)
		{}

		virtual std::string ToString() const override {
			return "Alloca()";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::Alloca;
		}
	};

	struct IRReturn : public IRInstruction {
		u64 Type = 0;
		IRInstruction* Value = nullptr;

		virtual std::string ToString() const override {
			std::string str;

			str += "ret ";
			str += std::to_string(Type);
			str += " : ";
			str += Value->ToString();

			return str;
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::Return;
		}
	};

	struct IRFunction : public IRInstruction {
		u64 ID = 0;

		TypeStorage* Overload = nullptr;
		std::vector<IRSSA*> Arguments;
		std::vector<IRInstruction*> Instructions;

		virtual std::string ToString() const override
		{
			std::string str = "fn $";

			str += std::to_string(ID) + " i32 : (";

			for (auto a : Arguments) {
				IRSSA* arg = (IRSSA*)a;
				str += "$ ";
				str += " : ";
				str += std::to_string(arg->ID);
				str += ", ";
			}

			str += ") {\n";

			for (auto inst : Instructions) {
				str += inst->ToString() + '\n';
			}

			str += "}\n";

			return str;
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::Function;
		}
	};

	struct IRTranslationUnit : public IRInstruction {
		u64 ID = 0;
		std::vector<IRInstruction*> Instructions;

		virtual IRNodeType GetType() const {
			return IRNodeType::TranslationUnit;
		}
	};

	struct IRFile : public IRInstruction {
		u64 ID = 0;

		std::string File_Name;
		std::string Directory;

		std::vector<IRInstruction*> Instructions;

		virtual IRNodeType GetType() const {
			return IRNodeType::File;
		}
	};

	struct IRFunctionCall : public IRInstruction {
		u64 ID = 0;
		u64 FuncID = 0;
		TypeStorage* Overload = nullptr;

		std::vector<IRInstruction*> Arguments;

		IRFunctionCall() = default;

		IRFunctionCall(std::vector<IRInstruction*> arguments, u64 funcID)
			:Arguments(arguments), FuncID(funcID)
		{
		}

		virtual std::string ToString() const override {
			return 	"CALL $" + std::to_string(FuncID) + "()";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::Call;
		}
	};

	struct IRStore : public IRInstruction {
		u64 ID = 0;

		u64 AddressSSA = 0;
		IRInstruction* Data = nullptr;

		TypeStorage* Type;

		IRStore() = default;

		IRStore(u64 addressSSA, IRInstruction* data, TypeStorage* type)
			:AddressSSA(addressSSA), Data(data), Type(type) {}

		virtual std::string ToString() const override {
			return fmt::format("STORE ${} {}", AddressSSA, Data->ToString());
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::Store;
		}
	};

	struct IRLoad : public IRInstruction {
		u64 ID = 0;

		u64 AddressSSA = 0;
		TypeStorage* Type;

		IRLoad() = default;

		IRLoad(u64 address_ssa, TypeStorage* type)
			:AddressSSA(address_ssa), Type(type)
		{}

		virtual std::string ToString() const override {
			return 	fmt::format("LOAD ${}", AddressSSA);
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::Load;
		}
	};

	struct IRData : public IRInstruction {
		u64 ID = 0;

		std::vector<char> Data;

		virtual std::string ToString() const override {
			return 	fmt::format("% {}", ID);
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::Data;
		}
	};

	struct IRDataValue : public IRInstruction {
		u64 ID = 0;
		u64 DataID = 0;

		IRDataValue(u64 id, u64 dataID)
			:ID(id), DataID(dataID)
		{
		}

		virtual std::string ToString() const override {
			return 	fmt::format("% {}", DataID);
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::DataValue;
		}
	};

	struct IRStructMember : public IRInstruction {
		u64 ID = 0;
		u64 TypeID = 0;

		u64 Pointer = 0;
		u64 Array = 0;

		virtual std::string ToString() const override {
			return 	"member " + std::to_string(TypeID) + ", ";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::StructMember;
		}
	};

	struct IRStruct : public IRInstruction {
		u64 ID = 0;
		u64 TypeID = 0;
		std::vector<IRStructMember*> Members;

		virtual std::string ToString() const override {
			return 	"struct";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::Struct;
		}
	};

	struct IRMemberAccess : public IRInstruction {
		u64 ID = 0;

		u64 StructID = 0;
		u64 ObjectSSA = 0;
		u64 MemberID = 0;

		bool ReferenceAccess = false;

		virtual std::string ToString() const override {
			return 	" -> ";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::MemberAccess;
		}
	};

	struct IRArrayAccess : public IRInstruction {
		u64 ID = 0;

		u64 ArrayAddress;
		u64 ElementSSA;
		TypeStorage* Type;

		IRArrayAccess() = default;

		IRArrayAccess(u64 array_address, u64 element_ssa, TypeStorage* type)
			:ArrayAddress(array_address), ElementSSA(element_ssa), Type(type)
		{
		}

		virtual std::string ToString() const override {
			return 	" [] -> ";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::ArrayAccess;
		}
	};

	//To be changed to branch
	struct IRIf : public IRInstruction {
		u64 ID = 0;
		u64 SSA = 0;
		std::vector<IRInstruction*> Instructions;

		virtual std::string ToString() const override
		{
			std::string str = "if";

			str += "{\n";

			for (auto inst : Instructions) {
				//str += inst->ToString() + '\n';
			}

			str += "}\n";

			return str;
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::If;
		}
	};

	//To be changed to branch
	struct IRWhile : public IRInstruction {
		u64 ID = 0;
		u64 SSA = 0;
		std::vector<IRSSA*> ConditionBlock;
		std::vector<IRInstruction*> Instructions;

		virtual std::string ToString() const override
		{
			std::string str = "while";

			str += "{\n";

			for (auto inst : Instructions) {
				str += inst->ToString() + '\n';
			}

			str += "}\n";

			return str;
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::While;
		}
	};

	struct IRSizeOF : public IRInstruction {
		u64 ID = 0;
		u64 Type = 0;

		IRSizeOF() = default;
		IRSizeOF(u64 type)
			: Type(type)
		{
		}

		virtual std::string ToString() const override
		{
			std::string str = "sizeof(";
			str += std::to_string(Type) + ")";
			return str;
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::SizeOf;
		}
	};

	struct IRBreak : public IRInstruction {
		u64 ID = 0;

		virtual std::string ToString() const override
		{
			return "break";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::Break;
		}
	};

	struct IRRef : public IRInstruction {
		u64 ID = 0;
		u64 SSA;

		virtual std::string ToString() const override
		{
			return "^";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::Ref;
		}
	};

	struct IRTypeOf : public IRInstruction {
		u64 ID = 0;

		TypeStorage* Type = 0;

		virtual std::string ToString() const override
		{
			return "typeof()";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::TypeOf;
		}
	};

	struct IRTypeInfo : public IRInstruction {
		u64 ID = 0;
		u64 ArgumentSSA = -1;

		IRTypeInfo(u64 argument)
			:ArgumentSSA(argument)
		{}

		virtual std::string ToString() const override
		{
			return "typeinfo()";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::TypeInfo;
		}
	};

	struct IREQ : public IRInstruction {
		IRSSAValue* SSA_A = nullptr;
		IRSSAValue* SSA_B = nullptr;
		u64 Type = -1;

		IREQ() = default;
		IREQ(IRSSAValue* A, IRSSAValue* B)
			:SSA_A(A), SSA_B(B)
		{
		}

		virtual std::string ToString() const override {
			std::string str;

			str += "EQ ";

			str += SSA_A->ToString();
			str += " : ";
			str += SSA_B->ToString();

			return str;
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::Equal;
		}
	};

	struct IRNOTEQ : public IRInstruction {
		IRSSAValue* SSA_A = nullptr;
		IRSSAValue* SSA_B = nullptr;
		u64 Type = -1;

		IRNOTEQ() = default;
		IRNOTEQ(IRSSAValue* A, IRSSAValue* B)
			:SSA_A(A), SSA_B(B)
		{
		}

		virtual std::string ToString() const override {
			std::string str;

			str += "NOTEQ ";

			str += SSA_A->ToString();
			str += " : ";
			str += SSA_B->ToString();

			return str;
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::NotEqual;
		}
	};

	struct IRGreater : public IRInstruction {
		IRSSAValue* SSA_A = nullptr;
		IRSSAValue* SSA_B = nullptr;
		u64 Type = -1;

		IRGreater() = default;
		IRGreater(IRSSAValue* A, IRSSAValue* B)
			:SSA_A(A), SSA_B(B)
		{
		}

		virtual std::string ToString() const override {
			std::string str;

			str += "Greater ";

			str += SSA_A->ToString();
			str += " : ";
			str += SSA_B->ToString();

			return str;
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::GreaterThan;
		}
	};

	struct IRLesser : public IRInstruction {
		IRSSAValue* SSA_A = nullptr;
		IRSSAValue* SSA_B = nullptr;
		u64 Type = -1;

		IRLesser() = default;
		IRLesser(IRSSAValue* A, IRSSAValue* B)
			:SSA_A(A), SSA_B(B)
		{
		}

		virtual std::string ToString() const override {
			std::string str;

			str += "Lesser ";

			str += SSA_A->ToString();
			str += " : ";
			str += SSA_B->ToString();

			return str;
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::LesserThan;
		}
	};

	struct IRBitAnd : public IRInstruction {
		IRSSAValue* SSA_A = nullptr;
		IRSSAValue* SSA_B = nullptr;

		IRBitAnd() = default;
		IRBitAnd(IRSSAValue* A, IRSSAValue* B)
			:SSA_A(A), SSA_B(B)
		{
		}

		virtual std::string ToString() const override {
			std::string str;

			str += "BitAnd ";

			str += SSA_A->ToString();
			str += " : ";
			str += SSA_B->ToString();

			return str;
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::BitAnd;
		}
	};

	struct IRBitOr : public IRInstruction {
		IRSSAValue* SSA_A = nullptr;
		IRSSAValue* SSA_B = nullptr;

		IRBitOr() = default;
		IRBitOr(IRSSAValue* A, IRSSAValue* B)
			:SSA_A(A), SSA_B(B)
		{
		}

		virtual std::string ToString() const override {
			std::string str;

			str += "BitOr ";

			str += SSA_A->ToString();
			str += " : ";
			str += SSA_B->ToString();

			return str;
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::BitOr;
		}
	};

	struct IRAnd : public IRInstruction {
		IRSSAValue* SSA_A = nullptr;
		IRSSAValue* SSA_B = nullptr;

		IRAnd() = default;
		IRAnd(IRSSAValue* A, IRSSAValue* B)
			:SSA_A(A), SSA_B(B)
		{
		}

		virtual std::string ToString() const override {
			std::string str;

			str += "And ";

			str += SSA_A->ToString();
			str += " : ";
			str += SSA_B->ToString();

			return str;
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::And;
		}
	};

	struct IROr : public IRInstruction {
		IRSSAValue* SSA_A = nullptr;
		IRSSAValue* SSA_B = nullptr;

		IROr() = default;
		IROr(IRSSAValue* A, IRSSAValue* B)
			:SSA_A(A), SSA_B(B)
		{
		}

		virtual std::string ToString() const override {
			std::string str;

			str += "Or ";

			str += SSA_A->ToString();
			str += " : ";
			str += SSA_B->ToString();

			return str;
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::Or;
		}
	};

	struct IRArrayAllocate : public IRInstruction {

		u64 ID;
		u64 Type;
		std::vector<u64> Data;

		virtual std::string ToString() const override {
			return "ArrayAllocate()";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::ArrayAllocate;
		}
	};

	struct IRDeRef : public IRInstruction {
		u64 ID = 0;
		u64 SSA = 0;

		IRDeRef(u64 ssa)
			:SSA(ssa) {}

		virtual std::string ToString() const override
		{
			return "<<";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::DeRef;
		}
	};

	struct IRFuncRef : public IRInstruction {
		u64 ID = 0;
		u64 FunctionID = 0;

		IRFuncRef(u64 functionID)
			:FunctionID(functionID) {}

		virtual std::string ToString() const override
		{
			return "&func()";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::FuncRef;
		}
	};

	struct IRCallFuncRef : public IRInstruction {
		u64 ID = 0;

		u64 PtrSSA = 0;
		std::vector<u64> Arguments;
		TypeStorage* Signature = 0;

		IRCallFuncRef(u64 ptrSSA, const std::vector<u64>& arguments, TypeStorage* signature)
			:PtrSSA(ptrSSA), Arguments(arguments), Signature(signature) {}

		virtual std::string ToString() const override
		{
			return "void*()";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::CallFuncRef;
		}
	};

	struct IRGlobalAddress : public IRInstruction {
		u64 ID = 0;
		u64 GlobID = 0;

		IRGlobalAddress(u64 glob_id)
			:GlobID(glob_id) {}

		virtual std::string ToString() const override
		{
			return "global $var";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::GlobAddress;
		}
	};

	struct IRGlobalDecl : public IRInstruction {
		u64 ID = 0;
		u64 GlobID = 0;

		TypeStorage* Type = nullptr;
		IRCONSTValue* Initializer = nullptr;

		virtual std::string ToString() const override
		{
			return "decl global var";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::GlobDecl;
		}
	};

	struct IRCast : public IRInstruction {
		u64 ID = 0;

		TypeStorage* Type = nullptr;
		u64 SSA = 0;

		virtual std::string ToString() const override {
			return 	"(invalid cast) ";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::Invalid;
		}
	};

	struct IRPointerCast : public IRInstruction {
		u64 ID = 0;

		TypeStorage* Type = nullptr;
		u64 PointerSSA = 0;

		IRPointerCast(TypeStorage* type, u64 pointer_ssa)
			:Type(type), PointerSSA(pointer_ssa)
		{}

		virtual std::string ToString() const override {
			return 	"(ptrcast) ";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::PointerCast;
		}
	};

	struct IRInt2PtrCast : public IRInstruction {
		u64 ID = 0;

		TypeStorage* Type = nullptr;
		u64 IntegerSSA = 0;

		IRInt2PtrCast(TypeStorage* type, u64 integer_ssa)
			:Type(type), IntegerSSA(integer_ssa)
		{}

		virtual std::string ToString() const override {
			return 	"(int 2 ptr) ";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::Int2PtrCast;
		}
	};

	struct IRPtr2IntCast : public IRInstruction {
		u64 ID = 0;

		TypeStorage* Type = nullptr;
		u64 PointerSSA = 0;

		IRPtr2IntCast(TypeStorage* type, u64 pointer_ssa)
			:Type(type), PointerSSA(pointer_ssa)
		{}

		virtual std::string ToString() const override {
			return 	"(ptr2 int) ";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::Ptr2IntCast;
		}
	};

	struct IRZExtCast : public IRInstruction {
		u64 ID = 0;

		TypeStorage* Type = nullptr;
		u64 IntegerSSA = 0;

		IRZExtCast(TypeStorage* type, u64 integerSSA)
			:Type(type), IntegerSSA(integerSSA)
		{}

		virtual std::string ToString() const override {
			return 	"zext() ";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::ZExtCast;
		}
	};

	struct IRSExtCast : public IRInstruction {
		u64 ID = 0;

		TypeStorage* Type = nullptr;
		u64 IntegerSSA = 0;

		IRSExtCast(TypeStorage* type, u64 integerSSA)
			:Type(type), IntegerSSA(integerSSA)
		{}

		virtual std::string ToString() const override {
			return 	"zext() ";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::SExtCast;
		}
	};

	struct IRIntTrunc : public IRInstruction {
		u64 ID = 0;

		TypeStorage* Type = nullptr;
		u64 IntegerSSA = 0;

		IRIntTrunc(TypeStorage* type, u64 integerSSA)
			:Type(type), IntegerSSA(integerSSA)
		{}

		virtual std::string ToString() const override {
			return 	"itrunc() ";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::IntTrunc;
		}
	};

	struct IRInt2FP : public IRInstruction {
		u64 ID = 0;

		TypeStorage* Type = nullptr;
		u64 IntegerSSA = 0;
		bool Signed = false;

		IRInt2FP(TypeStorage* type, u64 integerSSA, bool _signed)
			:Type(type), IntegerSSA(integerSSA), Signed(_signed)
		{}

		virtual std::string ToString() const override {
			return 	"itrunc() ";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::Int2FP;
		}
	};

	struct IRFP2Int : public IRInstruction {
		u64 ID = 0;

		TypeStorage* Type = nullptr;
		u64 IntegerSSA = 0;
		bool Signed = false;

		IRFP2Int(TypeStorage* type, u64 integerSSA, bool _signed)
			:Type(type), IntegerSSA(integerSSA), Signed(_signed)
		{}

		virtual std::string ToString() const override {
			return 	"itrunc() ";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::FP2Int;
		}
	};

	struct IRFPExt : public IRInstruction {
		u64 ID = 0;

		TypeStorage* Type = nullptr;
		u64 FloatSSA = 0;

		IRFPExt(TypeStorage* type, u64 floatSSA)
			:Type(type), FloatSSA(floatSSA)
		{}

		virtual std::string ToString() const override {
			return 	"fpext() ";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::FPExt;
		}
	};

	struct IRFPTrunc : public IRInstruction {
		u64 ID = 0;

		TypeStorage* Type = nullptr;
		u64 FloatSSA = 0;

		IRFPTrunc(TypeStorage* type, u64 floatSSA)
			:Type(type), FloatSSA(floatSSA)
		{}

		virtual std::string ToString() const override {
			return 	"ftrunc() ";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::FPTrunc;
		}
	};

	struct IRAny : public IRInstruction {
		u64 ID = 0;

		u64 DataSSA;
		TypeStorage* Type;

		IRAny(u64 data, TypeStorage* type_id)
			: DataSSA(data), Type(type_id)
		{
		}

		virtual std::string ToString() const override {
			return fmt::format("Any ({})", DataSSA);
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::Any;
		}
	};

	struct IRAnyArray : public IRInstruction {
		u64 ID = 0;

		std::vector<IRAny> Arguments;

		IRAnyArray(const std::vector<IRAny>& args)
			: Arguments(args)
		{
		}

		virtual std::string ToString() const override {
			return fmt::format("Any [{},...]", Arguments.size());
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::AnyArray;
		}
	};

	struct IRLabel : public IRInstruction {

		u64 LabelID = 0;
		std::vector<IRInstruction*> Instructions;

		virtual std::string ToString() const override {
			return "label:";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::Label;
		}
	};

	struct IRIterator : public IRInstruction {
		u64 ID = 0;

		std::vector<IRSSA*> ConditionBlock;
		std::vector<IRSSA*> IncrementorBlock;
		u64 ConditionSSA;

		IRSSAValue* IteratorIndex;
		IRSSAValue* IteratorIt;

		TypeStorage* IndexTy = nullptr;
		TypeStorage* ItTy = nullptr;

		virtual std::string ToString() const override {
			return "Iterator + ";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::Iterator;
		}
	};
}
