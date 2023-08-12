#pragma once

#include "FrontEnd/CompilerFile.h"
#include "BackEnd/IR.h"
#include "FrontEnd/Ast.h"

namespace Glass
{

	using TypeFlags = u64;

	enum TypeFlag {
		FLAG_BASE_TYPE = BIT(0),
		FLAG_NUMERIC_TYPE = BIT(1),
	};

	struct ContextScope {
		u64 ID = 0;
		std::vector<ContextScope> m_Children;
	};

	enum class SymbolType {
		None = 0,
		Variable,
		Function,
		Type,
		Enum,
	};

	enum class MessageType
	{
		Info,
		Warning,
		Error,
	};

	struct EnumMemberMetadata
	{
		std::string Name;
		u64 Value = 0;
	};

	struct EnumMetadata
	{
		Token Name;

		std::unordered_map<std::string, u64> MemberIndices;
		std::vector<EnumMemberMetadata> Members;

		void InsertMember(const std::string& name, const EnumMemberMetadata& member) {
			Members.push_back(member);
			MemberIndices[name] = Members.size() - 1;
		}

		const EnumMemberMetadata* GetMember(const std::string& member) const {
			auto it = MemberIndices.find(member);
			if (it != MemberIndices.end()) {
				return &Members[it->second];
			}

			return nullptr;
		}
	};

	struct CompilerMessage
	{
		std::string message;
		MessageType Type;
	};

	enum class TypeType
	{
		Value = 0,
		Pointer,
	};

	struct Type
	{
		u64 ID;
		bool Array = false;
		u64 Pointer = 0;

		Type* Ext = nullptr;
	};

	struct FunctionType {
		std::vector <Type*> Arguments;
		Type ReturnType;
	};

	struct VariableMetadata
	{
		Token Name;
		Type Tipe;
		bool IsArg = false;

		IRSSA* DataSSA = nullptr;
		IRSSA* AddressSSA = nullptr;
	};

	struct MemberMetadata
	{
		Token Name;
		Type Tipe;
	};

	struct StructMetadata
	{
		Token Name;
		std::vector<MemberMetadata> Members;

		bool Foreign = false;

		u64 FindMember(const std::string& name) const {
			u64 id_counter = 0;
			for (const MemberMetadata& member : Members) {
				if (member.Name.Symbol == name) {
					return id_counter;
				}
				id_counter++;
			}
			return (u64)-1;
		}
	};

	struct ArgumentMetadata
	{
		std::string Name;
		Type Tipe;
		bool Variadic = false;

		bool PolyMorphic = false;
		u64 PolyMorhID = 0;
	};

	struct PolyMorphicType
	{
		u64 ID;
	};

	struct PolyMorphOverloads
	{
		std::vector<std::pair<PolyMorphicType, Type>> TypeArguments;

		bool operator==(const PolyMorphOverloads& other) const
		{
			if (other.TypeArguments.size() != other.TypeArguments.size())
				return false;

			for (size_t i = 0; i < other.TypeArguments.size(); i++)
			{
				const auto& [other_a, other_b] = other.TypeArguments[i];
				const auto& [a, b] = TypeArguments[i];

				if (a.ID != other_a.ID) {
					return false;
				}

				if (b.ID != other_b.ID) {
					return false;
				}

				if (b.Pointer != other_b.Pointer) {
					return false;
				}

				if (b.Array != other_b.Array) {
					return false;
				}
			}

			return true;
		}
	};

	struct PolyMorphOverloadsHasher
	{
		std::size_t operator()(const PolyMorphOverloads& key) const {
			size_t hash = 0;
			for (const auto& [T, arg] : key.TypeArguments) {
				size_t a = arg.ID + arg.Array + (u64)arg.Pointer;
				hash += a / 3;
			}
			return hash;
		}
	};

	struct FunctionMetadata
	{
		std::string Name;

		std::vector<ArgumentMetadata> Arguments;

		Type ReturnType;

		bool Variadic = false;
		bool Foreign = false;
		bool PolyMorphic = false;

		FunctionNode* FunctionAST = nullptr;

		std::vector <std::pair<IRFunction*, FunctionNode*>> PendingPolymorphInstantiations;

		std::unordered_map<PolyMorphOverloads, IRFunction*, PolyMorphOverloadsHasher> PolyMorhOverLoads;

		std::unordered_map<std::string, u64> PolyMorphicTypeNames;
		std::unordered_map<u64, std::string> PolyMorphicIDToTypeNames;

		u64 GetPolyMorphID(const std::string& type_name) {
			if (auto it = PolyMorphicTypeNames.find(type_name) != PolyMorphicTypeNames.end()) {
				return it;
			}
			PolyMorphicTypeNames[type_name] = PolyMorphicTypeNames.size() + 1;

			PolyMorphicIDToTypeNames[PolyMorphicTypeNames[type_name]] = type_name;

			return GetPolyMorphID(type_name);
		}

		const ArgumentMetadata* GetArgument(u64 i) const
		{
			if (i > Arguments.size() - 1) {
				return nullptr;
			}
			else {
				return &Arguments[i];
			}
		}
	};

	struct OperatorQuery
	{
		std::vector<Type> TypeArguments;

		//@TODO: Add return types

		bool operator==(const OperatorQuery& other) const
		{
			if (other.TypeArguments.size() != other.TypeArguments.size())
				return false;

			for (size_t i = 0; i < other.TypeArguments.size(); i++)
			{
				const auto& oth = other.TypeArguments[i];
				const auto& ths = TypeArguments[i];

				if (ths.ID != oth.ID) {
					return false;
				}

				if (ths.ID != oth.ID) {
					return false;
				}

				if (ths.Pointer != oth.Pointer) {
					return false;
				}

				if (ths.Array != oth.Array) {
					return false;
				}
			}

			return true;
		}
	};

	struct OperatorQueryHasher
	{
		std::size_t operator()(const OperatorQuery& key) const {
			size_t hash = 0;
			for (const auto& arg : key.TypeArguments) {
				size_t a = arg.ID + arg.Array + (u64)arg.Pointer;
				hash += a / 3;
			}
			return hash;
		}
	};

	class Compiler
	{
	public:

		struct MetaData
		{
			u64 m_CurrentFunction = 0;

			std::unordered_map<u64, std::unordered_map<u64, IRSSA*>> m_SSAs;

			std::unordered_map<u64, std::unordered_map<std::string, u64>> m_VariableSSAs;
			std::unordered_map<u64, std::unordered_map<std::string, u64>> m_Variables;
			std::unordered_map<u64, std::unordered_map<u64, VariableMetadata >> m_VariableMetadata;

			std::unordered_map<u64, FunctionMetadata> m_Functions;
			std::unordered_map<std::string, u64> m_FunctionNames;

			std::unordered_map<u64, std::string> m_Types;
			std::unordered_map<u64, TypeFlags> m_TypeFlags;
			std::unordered_map<std::string, u64> m_TypeNames;
			std::unordered_map<u64, u64> m_TypeSizes;

			std::unordered_map<u64, u64> m_TypeToStruct;
			std::unordered_map<std::string, u64> m_StructNames;
			std::unordered_map<u64, StructMetadata> m_StructMetadata;

			std::unordered_map<u64, u64> m_TypeToEnum;
			std::unordered_map<std::string, u64> m_EnumNames;
			std::unordered_map<u64, EnumMetadata> m_Enums;

			std::unordered_map<u64, std::unordered_map<u64, Type>> m_ExpressionType;

			std::unordered_map<Operator, std::unordered_map<OperatorQuery, u64, OperatorQueryHasher>> m_Operators;

			void RegisterOperator(Operator op, const OperatorQuery& query, u64 function_id) {
				m_Operators[op][query] = function_id;
			}

			u64 GetOperator(Operator op, const OperatorQuery& query) {
				auto it = m_Operators[op].find(query);
				if (it != m_Operators[op].end()) {
					return it->second;
				}

				return (u64)-1;
			}

			TypeFlags& GetTypeFlags(u64 id) {
				return m_TypeFlags[id];
			}

			IRSSA* GetSSA(u64 ID) const {
				return m_SSAs.at(m_CurrentFunction).at(ID);
			}

			FunctionMetadata* GetFunctionMetadata(u64 ID) {
				if (m_Functions.find(ID) != m_Functions.end()) {
					return &m_Functions.at(ID);
				}

				return nullptr;
			}

			const FunctionMetadata* GetFunctionMetadata(u64 ID) const {
				if (m_Functions.find(ID) != m_Functions.end()) {
					return &m_Functions.at(ID);
				}

				return nullptr;
			}

			u64 GetFunctionMetadata(const std::string& name) const {
				if (m_FunctionNames.find(name) != m_FunctionNames.end()) {
					return m_FunctionNames.at(name);
				}

				return (u64)-1;
			}

			const std::string& GetType(u64 ID) const {
				return m_Types.at(ID);
			}

			const u64 GetType(const std::string& type_name) const {
				if (m_TypeNames.find(type_name) != m_TypeNames.end()) {
					return m_TypeNames.at(type_name);
				}
				else {
					return (u64)-1;
				}
			}

			const u64 GetTypeSize(u64 id) const {
				if (m_TypeSizes.find(id) != m_TypeSizes.end()) {
					return m_TypeSizes.at(id);
				}
				else {
					return (u64)-1;
				}
			}

			u64 GetVariableSSA(const std::string& name) const {
				if (m_VariableSSAs.find(m_CurrentFunction) != m_VariableSSAs.end()) {
					if (m_VariableSSAs.at(m_CurrentFunction).find(name) != m_VariableSSAs.at(m_CurrentFunction).end()) {
						return m_VariableSSAs.at(m_CurrentFunction).at(name);
					}
				}

				return (u64)-1;
			}

			u64 GetVariable(const std::string& name) const {
				if (m_Variables.find(m_CurrentFunction) != m_Variables.end()) {
					if (m_Variables.at(m_CurrentFunction).find(name) != m_Variables.at(m_CurrentFunction).end()) {
						return m_Variables.at(m_CurrentFunction).at(name);
					}
				}

				return (u64)-1;
			}

			void RegisterVariableMetadata(u64 ID, const VariableMetadata& metadata) {
				m_VariableMetadata[m_CurrentFunction][ID] = metadata;
				m_Variables[m_CurrentFunction][metadata.Name.Symbol] = ID;
			}

			const VariableMetadata* GetVariableMetadata(u64 ID) const {
				if (m_VariableMetadata.find(m_CurrentFunction) != m_VariableMetadata.end()) {
					if (m_VariableMetadata.at(m_CurrentFunction).find(ID) != m_VariableMetadata.at(m_CurrentFunction).end()) {
						return &m_VariableMetadata.at(m_CurrentFunction).at(ID);
					}
				}
				return nullptr;
			}

			u64 UpdateVariableSSA(const std::string& name, IRSSA* ssa) {
				return m_VariableSSAs.at(m_CurrentFunction).at(name) = ssa->ID;
			}

			u64 RegisterVariable(IRSSA* ssa, const std::string& name) {
				m_SSAs[m_CurrentFunction][ssa->ID] = ssa;
				return m_VariableSSAs[m_CurrentFunction][name] = ssa->ID;
			}

			void RegisterSSA(IRSSA* ssa) {
				m_SSAs[m_CurrentFunction][ssa->ID] = ssa;
			}

			void RegisterFunction(u64 ID, const std::string& name, Type returnType = {}, std::vector<ArgumentMetadata> args = {}, bool variadic = false) {
				FunctionMetadata func;
				func.Name = name;
				func.ReturnType = returnType;
				func.Arguments = args;
				func.Variadic = variadic;

				m_Functions[ID] = func;
				m_FunctionNames[name] = ID;
			}

			void RegisterType(u64 ID, const std::string& name, u64 size) {
				m_Types[ID] = name;
				m_TypeNames[name] = ID;
				m_TypeSizes[ID] = size;
			}

			u64 ComputeStructSize(const StructMetadata* metadata) {

				u64 size = 0;

				for (const MemberMetadata& member : metadata->Members) {
					if (!member.Tipe.Pointer && !member.Tipe.Array) {
						size += GetTypeSize(member.Tipe.ID);
					}
					else if (member.Tipe.Pointer) {
						size += 8;
					}
					else if (member.Tipe.Array) {
						size += 16;
					}
				}

				return size;
			}

			void RegisterStruct(u64 ID, u64 TypeID, const StructMetadata& metadata)
			{
				m_StructNames[metadata.Name.Symbol] = ID;
				m_StructMetadata[ID] = metadata;
				m_TypeToStruct[TypeID] = ID;

				RegisterType(TypeID, metadata.Name.Symbol, ComputeStructSize(&metadata));
			}

			const StructMetadata* GetStructMetadata(u64 ID) const
			{
				return &m_StructMetadata.at(ID);
			}

			const StructMetadata* GetStructFromType(u64 ID) const
			{
				return &m_StructMetadata.at(m_TypeToStruct.at(ID));
			}

			u64 GetStructIDFromType(u64 ID) const
			{
				auto it = m_TypeToStruct.find(ID);
				if (it != m_TypeToStruct.end()) {
					return it->second;
				}

				return (u64)-1;
			}

			const Type& GetExprType(u64 ssa) const {
				return m_ExpressionType.at(m_CurrentFunction).at(ssa);
			}

			void RegExprType(u64 ssa, const Type& type) {
				m_ExpressionType[m_CurrentFunction][ssa] = type;
			}

			SymbolType GetSymbolType(const std::string& symbol) {
				if (GetFunctionMetadata(symbol) != (u64)-1) {
					return SymbolType::Function;
				}

				if (GetVariableMetadata(GetVariableSSA(symbol)) != nullptr) {
					return SymbolType::Variable;
				}

				if (GetEnum(symbol) != nullptr) {
					return SymbolType::Enum;
				}

				if (GetType(symbol) != (u64)-1) {
					return SymbolType::Type;
				}

				return SymbolType::None;
			}

			void RegisterEnum(u64 ID, u64 TypeID, const EnumMetadata& metadata)
			{
				m_EnumNames[metadata.Name.Symbol] = ID;
				m_Enums[ID] = metadata;
				m_TypeToEnum[TypeID] = ID;

				//@TODO: Set correct enum size
				RegisterType(TypeID, metadata.Name.Symbol, 8);
				m_TypeFlags[TypeID] |= FLAG_NUMERIC_TYPE;
			}

			const EnumMetadata* GetEnumFromType(u64 type_id) {
				auto it = m_TypeToEnum.find(type_id);

				if (it != m_TypeToEnum.end()) {
					return &m_Enums[it->second];
				}

				return nullptr;
			}

			const EnumMetadata* GetEnum(u64 enum_id) {
				auto it = m_Enums.find(enum_id);

				if (it != m_Enums.end()) {
					return &it->second;
				}

				return nullptr;
			}

			const EnumMetadata* GetEnum(const std::string& name) {
				auto it = m_EnumNames.find(name);

				if (it != m_EnumNames.end()) {
					return &m_Enums[it->second];
				}

				return nullptr;
			}

		};

		Compiler(std::vector<CompilerFile*> files);

		IRTranslationUnit* CodeGen();

		IRInstruction* StatementCodeGen(const Statement* statement);

		IRInstruction* ForeignCodeGen(const ForeignNode* expression);

		IRInstruction* OperatorCodeGen(const OperatorNode* op_node);

		IRInstruction* FunctionCodeGen(FunctionNode* functionNode);

		IRInstruction* VariableCodeGen(const VariableNode* variableNode);

		IRInstruction* ReturnCodeGen(const ReturnNode* returnNode);
		IRInstruction* StructCodeGen(const StructNode* structNode);

		IRInstruction* EnumCodeGen(const EnumNode* enumNode);

		IRInstruction* IfCodeGen(const IfNode* ifNode);
		IRInstruction* WhileCodeGen(const WhileNode* ifNode);

		IRInstruction* ExpressionCodeGen(const Expression* expression);

		IRInstruction* IdentifierCodeGen(const Identifier* identifier);
		IRInstruction* NumericLiteralCodeGen(const NumericLiteral* numericLiteral);
		IRInstruction* StringLiteralCodeGen(const StringLiteral* stringLiteral);
		IRInstruction* BinaryExpressionCodeGen(const BinaryExpression* binaryExpr);
		IRInstruction* AssignmentCodeGen(const BinaryExpression* binaryExpr);
		IRInstruction* FunctionCallCodeGen(const FunctionCall* call);
		IRInstruction* MemberAccessCodeGen(const MemberAccess* memberAccess);
		IRInstruction* EnumMemberAccessCodeGen(const MemberAccess* memberAccess);

		IRInstruction* ArrayAccessCodeGen(const ArrayAccess* arrayAccess);
		IRInstruction* TypeofCodeGen(const TypeOfNode* typeof);
		IRInstruction* CastCodeGen(const CastNode* typeof);

		IRInstruction* RefCodeGen(const RefNode* refNode);
		IRInstruction* DeRefCodeGen(const DeRefNode* deRefNode);

		IRSSAValue* GetExpressionByValue(const Expression* expr);
		IRSSAValue* PassAsAny(const Expression* expr);
		IRSSAValue* PassAsVariadicArray(u64 start, const std::vector<Expression*>& arguments, const ArgumentMetadata* decl_arg);

		IRFunction* CreateIRFunction(const FunctionNode* functionNode);
		IRSSA* CreateIRSSA();
		IRData* CreateIRData();

		IRFunction* CreatePolyMorhOverload(u64 ID, const PolyMorphOverloads& overloads);

		const IRFunction* GetPolyMorphOverLoad(u64 ID, const PolyMorphOverloads& overloads)
		{
			auto metadata = m_Metadata.GetFunctionMetadata(ID);
			if (metadata->PolyMorhOverLoads.find(overloads) != metadata->PolyMorhOverLoads.end()) {
				return metadata->PolyMorhOverLoads.at(overloads);
			}
			else {
				IRFunction* ir_func = CreatePolyMorhOverload(ID, overloads);
				if (ir_func == nullptr) {
					return nullptr;
				}
				else {
					metadata->PolyMorhOverLoads.emplace(overloads, ir_func);
				}
			}
			return GetPolyMorphOverLoad(ID, overloads);
		}

		bool CheckTypeConversion(u64 a, u64 b)
		{
			const static std::unordered_map<u64, u64> numericTypes =
			{
				{IR_int,0},
				{IR_float,0},

				{IR_i8,0},
				{IR_i16,0},
				{IR_i32,0},
				{IR_i64,0},

				{IR_u8,0},
				{IR_u16,0},
				{IR_u32,0},
				{IR_u64,0},

				{IR_f32,0},
				{IR_f64,0},
			};

			bool both_numeric = false;

			bool a_numeric = numericTypes.find(a) != numericTypes.end();
			bool b_numeric = numericTypes.find(b) != numericTypes.end();

			both_numeric = a_numeric && b_numeric;

			if (both_numeric) {
				return true;
			}

			return a == b;
		}

		const MetaData& GetMetadata() {
			return m_Metadata;
		}

		void PushIRData(IRData* data) {
			m_DataStack.push_back(data);
		}

		std::vector<IRData*> PoPIRData() {
			auto cpy = m_DataStack;
			m_DataStack.clear();
			return cpy;
		}

		void PushIRSSA(IRSSA* ssa) {
			if (m_Scope == 0) {
				m_SSAStack.push_back(ssa);
				m_Metadata.RegisterSSA(ssa);
			}
			else {
				m_SSAStacks[m_Scope].push_back(ssa);
				m_Metadata.RegisterSSA(ssa);
			}
		}

		void ResetSSAIDCounter() {
			m_SSAIDCounter = 1;
		}

		std::vector<IRSSA*> PoPIRSSA() {
			if (m_Scope == 0) {
				auto cpy = m_SSAStack;
				m_SSAStack.clear();
				return cpy;
			}
			else {
				auto cpy = m_SSAStacks[m_Scope];
				m_SSAStacks[m_Scope].clear();
				return cpy;
			}
		}

		void PushScope()
		{
			m_Scope++;
		}

		void PopScope()
		{
			m_Scope--;
		}

		void PushIRInstruction(IRInstruction* ssa) {
			m_InstructionStack.push_back(ssa);
		}

		IRSSA* GetSSA(u64 ID) {
			return m_Metadata.GetSSA(ID);
		}

		u64 GetVariableSSA(const std::string& name) {
			return m_Metadata.GetVariableSSA(name);
		}

		u64 UpdateVariableSSA(const std::string& name, IRSSA* ssa) {
			return m_Metadata.UpdateVariableSSA(name, ssa);
		}

		u64 RegisterVariable(IRSSA* ssa, const std::string& name) {
			return m_Metadata.RegisterVariable(ssa, name);
		}

		void PushMessage(CompilerMessage msg) {
			m_Messages.push_back(msg);
		}

		const std::vector<CompilerMessage>& GetMessages() const {
			return m_Messages;
		}

		std::string PrintTokenLocation(const Token& token)
		{
			return fmt::format("{}:{}:{}", m_Files[m_CurrentFile]->GetPath().string(), token.Line + 1, token.Begin);
		}

		std::string PrintType(const Type& type)
		{
			std::string arr_ptr;

			if (type.Array) {
				arr_ptr += "[]";
			}

			if (type.Pointer) {
				arr_ptr += "*";
			}

			return fmt::format("{}{}", m_Metadata.GetType(type.ID), arr_ptr);
		}

		u64 GetTypeID() {
			m_TypeIDCounter++;
			return m_TypeIDCounter;
		}

		u64 GetStructID() {
			m_StructIDCounter++;
			return m_StructIDCounter;
		}

		u64 GetEnumID() {
			m_EnumIDCounter++;
			return m_EnumIDCounter;
		}

		u64 GetFunctionID() {
			m_FunctionIDCounter++;
			return m_FunctionIDCounter;
		}

	private:

		MetaData m_Metadata;

		std::vector<CompilerFile*> m_Files;

		u64 m_FunctionIDCounter = 1;
		u64 m_SSAIDCounter = 1;
		u64 m_DATAIDCounter = 1;
		u64 m_TypeIDCounter = 99;
		u64 m_StructIDCounter = 99;
		u64 m_EnumIDCounter = 0;

		std::vector<CompilerMessage> m_Messages;
		u64 m_CurrentFile = 0;

		u64 m_Scope = 0;

		std::unordered_map<u64, std::vector<IRSSA*> > m_SSAStacks;

		std::vector<IRInstruction*> m_InstructionStack;
		std::vector<IRSSA*> m_SSAStack;
		std::vector<IRData*> m_DataStack;
	};
}