#pragma once

namespace Glass
{
	enum class IRType : u64
	{
		IR_void,
		IR_i8,
		IR_i16,
		IR_i32,
		IR_i64,

		IR_u8,
		IR_u16,
		IR_u32,
		IR_u64,
	};

	enum IRCType : u64
	{
		IR_void,
		IR_i8,
		IR_i16,
		IR_i32,
		IR_i64,

		IR_u8,
		IR_u16,
		IR_u32,
		IR_u64,
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

		virtual std::string ToString() const override {
			return 	"$" + std::to_string(SSA);
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::SSAValue;
		}
	};

	struct IRADD : public IRInstruction {
		IRSSAValue* SSA_A = nullptr;
		IRSSAValue* SSA_B = nullptr;
		u64 Type;

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

	struct IRSSA : public IRInstruction {
		u64 ID = 0;
		u64 Type;
		bool Pointer = false;
		IRInstruction* Value = nullptr;
		IRSSA* PointsTo = nullptr;

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

		virtual std::string ToString() const override {
			return 	"(int)$" + std::to_string(SSA);
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::AddressAsValue;
		}
	};

	struct IRAddressOf : public IRInstruction {
		u64 ID = 0;
		u64 SSA = 0;

		IRAddressOf(u64 ssa)
			: SSA(ssa)
		{
		}

		virtual std::string ToString() const override {
			return 	"&$" + std::to_string(SSA);
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::AddressOf;
		}
	};

	struct IRStore : public IRInstruction {
		u64 ID = 0;
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

		virtual std::string ToString() const override {
			return 	" -> ";
		}

		virtual IRNodeType GetType() const {
			return IRNodeType::MemberAccess;
		}
	};
}