#pragma once

#include "BackEnd/Type.h"
#include "Base/Hash.h"

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

		If,
		While,
		Return,
		Break,

		Function,
		ForeignFunction,
		FuncRef,
		CallFuncRef,

		ARGValue,
		Call,
		AsAddress,
		AddressAsValue,
		AddressOf,
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
		NullPtr,

		TranslationUnit,
		File,
	};

	struct IRInstruction {
		virtual std::string ToString() const {
			return "IR Instruction";
		}

		virtual IRNodeType GetType() const = 0;
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
		u64 SSA = 0;

		IRSSAValue() = default;
		IRSSAValue(u64 ssa)
			:SSA(ssa)
		{
		}

		virtual std::string ToString() const override {
			return 	"$" + std::to_string(SSA);
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::SSAValue;
		}
	};
	struct IRARGValue : public IRInstruction {
		u64 ID = 0;
		u64 SSA = 0;

		IRARGValue() = default;
		IRARGValue(u64 ssa)
			:SSA(ssa)
		{
		}

		virtual std::string ToString() const override {
			return 	"%%" + std::to_string(SSA);
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::ARGValue;
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

		std::vector<IRInstruction*> Arguments;

		virtual std::string ToString() const override {
			return 	"CALL $" + std::to_string(FuncID) + "()";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::Call;
		}
	};

	struct IRAddressOf : public IRInstruction {
		u64 ID = 0;
		IRInstruction* SSA = 0;

		IRAddressOf() = default;
		IRAddressOf(IRInstruction* ssa)
			:SSA(ssa)
		{
		}

		virtual std::string ToString() const override {
			return 	"&$" + SSA->ToString();
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::AddressOf;
		}
	};

	struct IRStore : public IRInstruction {
		u64 ID = 0;

		u64 AddressSSA = 0;
		IRInstruction* Data = nullptr;

		TypeStorage* Type;

		virtual std::string ToString() const override {
			return fmt::format("STORE ${} {}", AddressSSA, Data->ToString());
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::Store;
		}
	};

	struct IRLoad : public IRInstruction {
		u64 ID = 0;
		u64 SSAddress = 0;

		TypeStorage* Type;

		virtual std::string ToString() const override {
			return 	fmt::format("LOAD ${}", SSAddress);
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
		u64 Type;

		bool ReferenceAccess = false;

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

	struct IRPointerCast : public IRInstruction {
		u64 ID = 0;

		TypeStorage* Type = 0;
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
}
