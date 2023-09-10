#pragma once
#include "Base/Types.h"
#include "FrontEnd/Ast.h"
#include "FrontEnd/Parser.h"
#include "FrontEnd/Token.h"

#include "FrontEnd/CompilerFile.h"
#include "BackEnd/IR.h"
#include "FrontEnd/Ast.h"
#include "BackEnd/Type.h"

namespace Glass {

	struct TypeStorage;
	struct TSFunc;
	enum class TypeStorageKind : u32;

	using TypeFlags = u64;
	using TypeInfoFlags = u64;

	enum TypeInfoFlag {
		TI_BASE_TYPE = BIT(0),
		TI_NUMERIC_TYPE = BIT(1),
		TI_UNSIGNED_TYPE = BIT(2),
		TI_FLOATING_TYPE = BIT(3),
		TI_STRUCT = BIT(4),
		TI_STRUCT_MEMBER = BIT(5),
		TI_ENUM = BIT(6),
		TI_FUNCTION = BIT(7),
	};

	enum TypeFlag {
		FLAG_BASE_TYPE = BIT(0),
		FLAG_NUMERIC_TYPE = BIT(1),
		FLAG_UNSIGNED_TYPE = BIT(2),
		FLAG_FLOATING_TYPE = BIT(3),
		FLAG_STRUCT_TYPE = BIT(4),
		FLAG_ENUM_TYPE = BIT(5),
		FLAG_FUNCTION_TYPE = BIT(6),
	};

	enum class ContextScopeType {
		GLOB = 0,
		FUNC = 1,
		STRUCT = 2,
		ENUM = 3,
	};

	struct ContextScope {
		std::vector<u64> Children;
		u64 Parent;

		u64 ID = 0;
		ContextScopeType Type;
	};

	enum class SymbolType {
		None = 0,
		Variable,
		GlobVariable,
		Function,
		Type,
		Enum,
		Constant,
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

	struct VariableMetadata
	{
		Token Name;
		TypeStorage* Tipe = nullptr;

		bool IsArg = false;
		bool Global = false;
		bool Foreign = false;

		IRSSA* DataSSA = nullptr;
		IRSSA* AddressSSA = nullptr;
	};

	struct MemberMetadata
	{
		Token Name;
		TypeStorage* Type = nullptr;
	};

	struct StructMetadata
	{
		Token Name;
		std::vector<MemberMetadata> Members;

		bool Foreign = false;

		u64 TypeID = 0;

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
		ArgumentMetadata() = default;

		ArgumentMetadata(
			std::string name,
			TypeStorage* type,
			u64 ssa_id = 0,
			bool variadic = false,
			bool polyMorphic = false,
			u64 PolyMorhID = 0)
			:Name(name), SSAID(ssa_id), Variadic(variadic), PolyMorphic(polyMorphic), PolyMorhID(PolyMorhID)
		{}

		std::string Name;
		TypeStorage* Type = nullptr;
		bool Variadic = false;

		bool PolyMorphic = false;
		u64 PolyMorhID = 0;

		u64 SSAID = 0; // where this value ptr to stack is located could be a double pointer or more its always a pointer
	};

	struct PolyMorphicType
	{
		u64 ID;
	};

	struct FunctionMetadata
	{
		Token Symbol;
		std::vector<ArgumentMetadata> Arguments;
		TypeStorage* Signature = nullptr;
		TypeStorage* ReturnType = nullptr;

		bool HasBody = false;
		bool Variadic = false;
		bool Foreign = false;

		bool PolyMorphic = false;

		std::vector<IRFunction*> Instantiations;
		std::map<u64, IRFunction*> PolyMotphicOverloads;

		FunctionNode* Ast = nullptr;

		std::map<TSFunc*, FunctionMetadata> Overloads;
		std::map<TSFunc*, FunctionMetadata*> OverloadsArgLookUp;

		////////////////////////////////////////////////////////////
		///OVER LOADING/////////////////////////////////
		bool IsOverloaded() const;
		void AddOverload(const FunctionMetadata& function);
		FunctionMetadata* FindOverload(TSFunc* signature);
		//signature return type assumed to be void for this to work
		FunctionMetadata* FindOverloadForCall(TSFunc* signature);
		FunctionMetadata& GetOverload(TSFunc* signature);
		const FunctionMetadata& GetOverload(TSFunc* signature) const;
		////////////////////////////////////////////////////////////
		const ArgumentMetadata* GetArgument(u64 i) const;
		////////////////////////////////////////////////////////////
		///Poly Morphism/////////////////////////////////

		////////////////////////////////////////////////////////////
	};

	struct ConstantDecl {
		Token Name;
		IRInstruction* Value = nullptr;
		TypeStorage* Type = nullptr;
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

	struct MetaData
	{
		//TypeInfo

		TypeInfoFlags GetTypeInfoFlags(u64 Type) const
		{
			TypeInfoFlags type_info_flags = 0;
			TypeFlags type_flags = GetTypeFlags(Type);

			if (type_flags & TypeFlag::FLAG_BASE_TYPE) {
				type_info_flags |= TI_BASE_TYPE;
			}

			if (type_flags & TypeFlag::FLAG_UNSIGNED_TYPE) {
				type_info_flags |= TI_UNSIGNED_TYPE;
			}

			if (type_flags & TypeFlag::FLAG_NUMERIC_TYPE) {
				type_info_flags |= TI_NUMERIC_TYPE;
			}

			if (type_flags & TypeFlag::FLAG_FLOATING_TYPE) {
				type_info_flags |= TI_FLOATING_TYPE;
			}

			if (type_flags & TypeFlag::FLAG_STRUCT_TYPE) {
				type_info_flags |= TI_STRUCT;
			}

			if (type_flags & TypeFlag::FLAG_ENUM_TYPE) {
				type_info_flags |= TI_ENUM;
			}


			return type_info_flags;
		}

		///////////////

		u64 m_CurrentFunction = 0;

		u64 m_ScopeIDCounter = 0;
		u64 m_CurrentCTXScope = 0;

		std::unordered_map<u64, ContextScope> m_Scopes;

		u64 GetScopeID() {
			m_ScopeIDCounter++;
			return m_ScopeIDCounter;
		}

		const ContextScope* CurrentContext() const {
			return &m_Scopes.at(m_CurrentCTXScope);
		}

		ContextScope* CurrentContext() {
			return &m_Scopes.at(m_CurrentCTXScope);
		}

		const ContextScope* GetContext(u64 id) const {
			return &m_Scopes.at(id);
		}

		u64 CurrentContextID() const {
			return m_CurrentCTXScope;
		}

		void PushContextGlobal() {
			ContextScope ctx_scope;
			ctx_scope.ID = GetScopeID();
			ctx_scope.Parent = 0;
			ctx_scope.Type = ContextScopeType::GLOB;

			m_Scopes[ctx_scope.ID] = ctx_scope;
			m_CurrentCTXScope = ctx_scope.ID;
		}

		void PushContext(ContextScopeType scope_type) {
			ContextScope ctx_scope;
			ctx_scope.ID = GetScopeID();
			ctx_scope.Parent = CurrentContext()->ID;
			ctx_scope.Type = scope_type;

			m_Scopes[ctx_scope.ID] = ctx_scope;

			CurrentContext()->Children.push_back(ctx_scope.ID);

			m_CurrentCTXScope = ctx_scope.ID;
		}

		void PopContext() {
			GS_CORE_ASSERT(m_CurrentCTXScope != 1, "Cannot Pop Global Context");
			m_CurrentCTXScope = CurrentContext()->Parent;
		}

		std::unordered_map<u64, std::unordered_map<u64, IRSSA*>> m_SSAs;

		std::unordered_map<std::string, u64> m_GlobalVariables;

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

		std::unordered_map<u64, std::unordered_map<u64, TypeStorage*>> m_ExpressionType;

		std::unordered_map<Operator, std::unordered_map<OperatorQuery, u64, OperatorQueryHasher>> m_Operators;

		std::unordered_map<u64, std::unordered_map<std::string, ConstantDecl>> m_Constants;

		void InsertConstant(const std::string& name, const ConstantDecl& constant) {
			m_Constants[CurrentContextID()][name] = constant;
		}

		const ConstantDecl* GetConstant(const std::string& name, u64 ctxID = -1) const
		{
			if (ctxID == 0) {
				return nullptr;
			}

			if (ctxID == -1) {
				ctxID = CurrentContextID();
			}

			if (m_Constants.find(ctxID) == m_Constants.end()) {
				return GetConstant(name, GetContext(ctxID)->Parent);
			}

			auto it = m_Constants.at(ctxID).find(name);

			if (it != m_Constants.at(ctxID).end()) {
				return &it->second;
			}
			else if (GetContext(ctxID)->Parent != 0) {
				return GetConstant(name, GetContext(ctxID)->Parent);
			}

			return nullptr;
		}

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

		const TypeFlags& GetTypeFlags(u64 id) const {
			return m_TypeFlags.at(id);
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

		void RegisterFunction(u64 ID, const FunctionMetadata& metadata);

		const u64 GetTypeSize(u64 id) const {
			if (m_TypeSizes.find(id) != m_TypeSizes.end()) {
				return m_TypeSizes.at(id);
			}
			else {
				return (u64)-1;
			}
		}

		u64 RegisterVariable(IRSSA* ssa, const std::string& name) {
			m_SSAs[m_CurrentFunction][ssa->ID] = ssa;
			return m_VariableSSAs[CurrentContextID()][name] = ssa->ID;
		}

		u64 GetVariableSSARecursive(u64 ctx_id, const std::string& name) const {

			if (ctx_id == 1) {
				return (u64)-1;
			}

			if (m_VariableSSAs.find(ctx_id) != m_VariableSSAs.end()) {
				if (m_VariableSSAs.at(ctx_id).find(name) != m_VariableSSAs.at(ctx_id).end()) {
					return m_VariableSSAs.at(ctx_id).at(name);
				}
			}

			u64 parent_ctx = GetContext(ctx_id)->Parent;

			if (parent_ctx == 1) {
				return (u64)-1;
			}

			return GetVariableSSARecursive(parent_ctx, name);
		}

		u64 GetVariableSSA(const std::string& name) const {
			return GetVariableSSARecursive(CurrentContextID(), name);
		}

		u64 GetVariableRecursive(u64 ctx_id, const std::string& name) const {

			if (ctx_id == 0) {
				return (u64)-1;
			}

			auto it = m_Variables.find(ctx_id);

			if (it != m_Variables.end()) {
				auto itt = it->second.find(name);
				if (itt != it->second.end()) {
					return itt->second;
				}
			}

			if (GetContext(ctx_id)->Parent == 0) {
				return (u64)-1;
			}

			return GetVariableRecursive(GetContext(ctx_id)->Parent, name);
		}

		u64 GetVariable(const std::string& name) const {
			return GetVariableRecursive(CurrentContextID(), name);
		}

		//Var Metadata
		const VariableMetadata* GetVariableMetadataRecursive(u64 ctx_id, u64 ID) const {

			if (ctx_id == 0) {
				return nullptr;
			}

			if (m_VariableMetadata.find(ctx_id) != m_VariableMetadata.end()) {
				if (m_VariableMetadata.at(ctx_id).find(ID) != m_VariableMetadata.at(ctx_id).end()) {
					return &m_VariableMetadata.at(ctx_id).at(ID);
				}
			}

			if (GetContext(ctx_id)->Parent == 0) {
				return nullptr;
			}

			return GetVariableMetadataRecursive(GetContext(ctx_id)->Parent, ID);
		}

		void RegisterVariableMetadata(u64 ID, const VariableMetadata& metadata) {
			m_VariableMetadata[CurrentContextID()][ID] = metadata;
			m_Variables[CurrentContextID()][metadata.Name.Symbol] = ID;
		}

		const VariableMetadata* GetVariableMetadata(u64 ID) const {
			return GetVariableMetadataRecursive(CurrentContextID(), ID);
		}

		void RegisterSSA(IRSSA* ssa) {
			m_SSAs[m_CurrentFunction][ssa->ID] = ssa;
		}

		void RegisterGlobalVariable(u64 glob_id, const std::string& name) {
			m_GlobalVariables[name] = glob_id;
		}

		u64 GetGlobalVariable(const std::string& name) const {
			auto it = m_GlobalVariables.find(name);
			if (it != m_GlobalVariables.end()) {
				return it->second;
			}
			return (u64)-1;
		}

		void RegisterType(u64 ID, const std::string& name, u64 size) {
			m_Types[ID] = name;
			m_TypeNames[name] = ID;
			m_TypeSizes[ID] = size;
			m_TypeFlags[ID] = 0;
		}

		const u64 GetTypeSize(TypeStorage* type) const;

		u64 ComputeStructSize(const StructMetadata* metadata) {

			u64 size = 0;

			for (const MemberMetadata& member : metadata->Members) {
				size += GetTypeSize(member.Type);
			}

			return size;
		}

		void RegisterStruct(u64 ID, u64 TypeID, StructMetadata metadata)
		{
			metadata.TypeID = TypeID;

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

		TypeStorage* GetExprType(u64 ssa) const {
			return m_ExpressionType.at(m_CurrentFunction).at(ssa);
		}

		void RegExprType(u64 ssa, TypeStorage* type) {
			m_ExpressionType[m_CurrentFunction][ssa] = type;
		}

		SymbolType GetSymbolType(const std::string& symbol) const;

		void RegisterEnum(u64 ID, u64 TypeID, const EnumMetadata& metadata)
		{
			m_EnumNames[metadata.Name.Symbol] = ID;
			m_Enums[ID] = metadata;
			m_TypeToEnum[TypeID] = ID;

			//@TODO: Set correct enum size
			RegisterType(TypeID, metadata.Name.Symbol, 8);
			m_TypeFlags[TypeID] |= FLAG_NUMERIC_TYPE;
			m_TypeFlags[TypeID] |= FLAG_ENUM_TYPE;
		}

		const EnumMetadata* GetEnumFromType(u64 type_id) const {
			auto it = m_TypeToEnum.find(type_id);

			if (it != m_TypeToEnum.end()) {
				return &m_Enums.at(it->second);
			}

			return nullptr;
		}

		const EnumMetadata* GetEnum(u64 enum_id) const {
			auto it = m_Enums.find(enum_id);

			if (it != m_Enums.end()) {
				return &it->second;
			}

			return nullptr;
		}

		u64 GetEnumID(const std::string& name) const {
			auto it = m_EnumNames.find(name);
			if (it != m_EnumNames.end()) {
				return it->second;
			}
			return -1;
		}

		const EnumMetadata* GetEnum(const std::string& name) const {
			auto it = m_EnumNames.find(name);

			if (it != m_EnumNames.end()) {
				return &m_Enums.at(it->second);
			}

			return nullptr;
		}

		EnumMetadata* GetEnum(const std::string& name) {
			auto it = m_EnumNames.find(name);

			if (it != m_EnumNames.end()) {
				return &m_Enums.at(it->second);
			}

			return nullptr;
		}
	};
}