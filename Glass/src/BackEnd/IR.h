#pragma once

namespace Glass
{
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

		IR_typeinfo,
	};

	enum class IRNodeType
	{
		ConstValue,
		SSA,
		ADD,
		SUB,
		MUL,
		DIV,
		SSAValue,
		ARGValue,
		Function,
		Return,
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
		If,
		While,
		SizeOf,
		Break,
		Ref,
		TypeOf,
		Equal,
		NotEqual,
		GreaterThan,
		LesserThan,
		TranslationUnit,
	};

	struct IRInstruction {
		virtual std::string ToString() const {
			return "IR Instruction";
		}

		virtual IRNodeType GetType() const = 0;
	};

	struct IRCONSTValue : public IRInstruction {
		u64 ID = 0;
		uint8_t Data[16];
		u64 Type;

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

	struct IRSSA : public IRInstruction {
		u64 ID = 0;
		IRInstruction* Value = nullptr;

		u64 Type;
		bool Pointer = false;

		RegType SSAType = RegType::None;

		bool Reference = false;
		bool PointerReference = false;
		u64 ReferenceType = false;

		virtual std::string ToString() const override {
			std::string str = "$ ";

			str += std::to_string(Type);
			str += " : ";
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
	};

	struct IRReturn : public IRInstruction {
		u64 Type;
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
		std::vector<IRFunction*> Polymorphs;

		virtual std::string ToString() const override
		{
			std::string str = "fn $";

			str += std::to_string(ID) + " i32 : (";

			for (auto a : Arguments) {
				IRSSA* arg = (IRSSA*)a;
				str += "$ ";
				str += std::to_string(arg->Type);
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

	struct IRAsAddress : public IRInstruction {
		u64 ID = 0;
		u64 SSA = 0;

		virtual std::string ToString() const override {
			return 	"(int*)$" + std::to_string(SSA);
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::AsAddress;
		}
	};

	struct IRAddressAsValue : public IRInstruction {
		u64 ID = 0;
		u64 SSA = 0;

		IRAddressAsValue(u64 ssa)
			:SSA(ssa)
		{
		}

		virtual std::string ToString() const override {
			return 	"(int)$" + std::to_string(SSA);
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::AddressAsValue;
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
		u64 Type = 0;
		bool Pointer = 0;
		u64 AddressSSA = 0;
		IRInstruction* Data = nullptr;
		virtual std::string ToString() const override {
			return 	fmt::format("STORE ${} {}", AddressSSA, Data->ToString());
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::Store;
		}
	};

	struct IRLoad : public IRInstruction {
		u64 ID = 0;
		u64 SSAddress = 0;
		u64 Type = 0;
		bool ReferencePointer = false;

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

		bool Pointer = false;

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
				str += inst->ToString() + '\n';
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

	struct TypeInfo
	{
		u64 id;
		bool pointer = false;
		std::string name;
	};

	struct IRTypeOf : public IRInstruction {
		u64 ID = 0;
		u64 VariableID;
		TypeInfo Type;

		virtual std::string ToString() const override
		{
			return "typeof(" + std::to_string(Type.id) + ")";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::TypeOf;
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
}
