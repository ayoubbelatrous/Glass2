#include "pch.h"

#include "TypeSystem.h"

namespace Glass {

	TypeSystem::TypeSystem(const MetaData& metadata) :m_Metadata(metadata) {}

	TSDynArray* TypeSystem::GetDynArray(TypeStorage* element)
	{
		GS_CORE_ASSERT(element);

		u64 hash = DynArrayHash(element->Hash);

		auto it = m_Instance->m_Types.find(hash);

		if (it != m_Instance->m_Types.end()) {
			return (TSDynArray*)m_Instance->m_OrderedTypeArray[it->second];
		}

		TSDynArray* new_type = TYPE(TSDynArray());
		new_type->BaseID = element->BaseID;
		new_type->Kind = TypeStorageKind::DynArray;
		new_type->Hash = hash;
		new_type->ElementType = element;

		Insert(hash, new_type);

		return new_type;
	}

	void TypeSystem::Init(const MetaData& metadata)
	{
		GS_CORE_ASSERT(!m_Instance);
		m_Instance = new TypeSystem(metadata);
	}

	TypeFlags TypeSystem::GetTypeFlags(TypeStorage* type)
	{
		TypeFlags flags = 0;

		if (type->Kind == TypeStorageKind::Base) {
			flags |= m_Instance->m_Metadata.GetTypeFlags(type->BaseID);
		}

		return flags;
	}

	TypeStorage* TypeSystem::GetBasic(u64 type_id)
	{
		m_Instance->m_Metadata.GetType(type_id);

		u64 hash = BasicTypeHash((u32)type_id);

		auto it = m_Instance->m_Types.find(hash);

		if (it != m_Instance->m_Types.end()) {
			return m_Instance->m_OrderedTypeArray[it->second];
		}

		TypeStorage* new_type = TYPE(TypeStorage());
		new_type->BaseID = (u32)type_id;
		new_type->Kind = TypeStorageKind::Base;
		new_type->Hash = hash;

		Insert(hash, new_type);

		return new_type;
	}

	TypeStorage* TypeSystem::GetBasic(const std::string& type_name)
	{
		u64 type_id = m_Instance->m_Metadata.GetType(type_name);
		if (type_id == -1) {
			return nullptr;
		}

		return GetBasic(type_id);
	}

	TSPtr* TypeSystem::GetPtr(TypeStorage* pointee, u32 indirection)
	{
		GS_CORE_ASSERT(indirection, "Cannot Have 0 Indirection");
		GS_CORE_ASSERT(pointee);

		u64 hash = PtrHash(pointee->Hash, indirection);

		auto it = m_Instance->m_Types.find(hash);

		if (it != m_Instance->m_Types.end()) {
			return (TSPtr*)m_Instance->m_OrderedTypeArray[it->second];
		}

		TSPtr* new_type = TYPE(TSPtr());
		new_type->BaseID = pointee->BaseID;
		new_type->Kind = TypeStorageKind::Pointer;
		new_type->Hash = hash;
		new_type->Pointee = pointee;
		new_type->Indirection = indirection;

		Insert(hash, new_type);

		return new_type;
	}

	TypeStorage* TypeSystem::GetFunction(const std::vector<TypeStorage*>& arguments, TypeStorage* return_type)
	{
		u64 hash = FunctionArgumentsHash(arguments, return_type);

		auto it = m_Instance->m_Types.find(hash);

		if (it != m_Instance->m_Types.end()) {
			return m_Instance->m_OrderedTypeArray[it->second];
		}

		TSFunc* new_type = TYPE(TSFunc());
		new_type->BaseID = -1;
		new_type->Kind = TypeStorageKind::Function;
		new_type->Hash = hash;
		new_type->Arguments = arguments;
		new_type->ReturnType = return_type;

		Insert(hash, new_type);

		return new_type;
	}

	TypeStorage* TypeSystem::GetVoid()
	{
		return GetBasic(IR_void);
	}

	TypeStorage* TypeSystem::GetBool()
	{
		return GetBasic(IR_bool);
	}

	TypeStorage* TypeSystem::GetU64()
	{
		return GetBasic(IR_u64);
	}

	TypeStorage* TypeSystem::GetU32()
	{
		return GetBasic(IR_u32);
	}

	TypeStorage* TypeSystem::GetU8()
	{
		return GetBasic(IR_u8);
	}

	TypeStorage* TypeSystem::GetI64()
	{
		return GetBasic(IR_i64);
	}

	TypeStorage* TypeSystem::GetI32()
	{
		return GetBasic(IR_i32);
	}

	TypeStorage* TypeSystem::GetI8()
	{
		return GetBasic(IR_i8);
	}

	TypeStorage* TypeSystem::GetVoidPtr()
	{
		return GetPtr(GetVoid(), 1);
	}

	TypeStorage* TypeSystem::GetAny()
	{
		return GetBasic(IR_any);
	}

	TypeStorage* TypeSystem::GetType()
	{
		return GetBasic(IR_type);
	}

	TypeStorage* TypeSystem::GetArray()
	{
		return GetBasic(IR_array);
	}

	TypeStorage* TypeSystem::IncreaseIndirection(TypeStorage* type)
	{
		if (type->Kind == TypeStorageKind::Pointer) {
			return TypeSystem::GetPtr(((TSPtr*)type)->Pointee, ((TSPtr*)type)->Indirection + 1);
		}
		else {
			return TypeSystem::GetPtr(type, 1);
		}
	}

	TypeStorage* TypeSystem::ReduceIndirection(TSPtr* pointer)
	{

		GS_CORE_ASSERT(pointer->Kind == TypeStorageKind::Pointer);

		if (pointer->Indirection - 1 != 0) {
			return TypeSystem::GetPtr(pointer->Pointee, pointer->Indirection - 1);
		}
		else {
			return pointer->Pointee;
		}
	}

	u16 TypeSystem::IndirectionCount(TypeStorage* type)
	{
		if (type->Kind != TypeStorageKind::Pointer) {
			return 0;
		}
		else {
			return ((TSPtr*)type)->Indirection;
		}
	}

	bool TypeSystem::IsPointer(TypeStorage* type)
	{
		return type->Kind == TypeStorageKind::Pointer;
	}

	bool TypeSystem::IsArray(TypeStorage* type)
	{
		return type->Kind == TypeStorageKind::DynArray;
	}

	TypeStorage* TypeSystem::GetArrayElementTy(TypeStorage* type)
	{
		if (type->Kind == TypeStorageKind::DynArray) {
			auto as_dyn_array = (TSDynArray*)type;
			return as_dyn_array->ElementType;
		}
		return nullptr;
	}

	std::vector<TypeStorage*>& TypeSystem::GetTypeMap()
	{
		return m_Instance->m_OrderedTypeArray;
	}

	u64 TypeSystem::GetTypeInfoIndex(TypeStorage* ts)
	{
		return m_Instance->m_Types.find(ts->Hash)->second;
	}

	bool TypeSystem::StrictPromotion(TypeStorage* A, TypeStorage* B)
	{
		if (A == B) return true;

		if (A->Kind == TypeStorageKind::Base && B->Kind == TypeStorageKind::Base) {

			auto A_TypeFlags = m_Instance->m_Metadata.GetTypeFlags(A->BaseID);
			auto B_TypeFlags = m_Instance->m_Metadata.GetTypeFlags(B->BaseID);

			auto A_TypeSize = m_Instance->m_Metadata.GetTypeSize(A->BaseID);
			auto B_TypeSize = m_Instance->m_Metadata.GetTypeSize(B->BaseID);

			if (A_TypeFlags & FLAG_BASE_TYPE) {
				if (A_TypeFlags & FLAG_NUMERIC_TYPE) {
					if (A_TypeFlags == B_TypeFlags) {
						if (B_TypeSize == A_TypeSize) {
							return true;
						}
					}
				}
			}
		}

		return false;
	}

	TypeStorage* TypeSystem::TypeExpressionGetType(TypeExpression* type_expr)
	{
		u16 indirection = 0;
		TypeStorage* type = nullptr;

		if (type_expr->GetType() == NodeType::TE_Pointer) {
			indirection = ((TypeExpressionPointer*)type_expr)->Indirection;
			type_expr = ((TypeExpressionPointer*)type_expr)->Pointee;
		}

		if (type_expr->GetType() == NodeType::TE_TypeName) {
			type = TypeSystem::GetBasic(((TypeExpressionTypeName*)type_expr)->Symbol.Symbol);
			if (!type) {
				return nullptr;
			}
		}

		if (type_expr->GetType() == NodeType::Identifier) {
			type = TypeSystem::GetBasic(((Identifier*)type_expr)->Symbol.Symbol);
		}

		if (type_expr->GetType() == NodeType::TE_Func) {
			TypeExpressionFunc* type_func = (TypeExpressionFunc*)type_expr;

			std::vector<TypeStorage*> arguments;

			for (auto arg : type_func->Arguments) {
				GS_CORE_ASSERT(arg);
				arguments.push_back(TypeExpressionGetType(arg));
			}

			TypeStorage* return_type = nullptr;
			if (type_func->ReturnType) {
				return_type = TypeExpressionGetType(type_func->ReturnType);
			}
			else {
				return_type = TypeSystem::GetBasic(IR_void);
			}

			type = TypeSystem::GetFunction(arguments, return_type);
		}

		if (type_expr->GetType() == NodeType::TE_Array) {
			TypeExpressionArray* type_array = (TypeExpressionArray*)type_expr;

			auto element = TypeExpressionGetType(type_array->ElementType);
			if (!element) return nullptr;

			type = TypeSystem::GetDynArray(element);
		}

		if (indirection) {
			type = TypeSystem::GetPtr(type, indirection);
		}

		return type;
	}

	u64 TypeSystem::GetTypeSize(TypeStorage* type)
	{
		if (type->Kind == TypeStorageKind::Pointer) {
			return 8;
		}

		if (type->Kind == TypeStorageKind::DynArray) {
			return 16;
		}

		if (type->Kind == TypeStorageKind::Function) {
			return 8;
		}

		if (type->Kind == TypeStorageKind::Base) {
			return m_Instance->m_Metadata.GetTypeSize(type->BaseID);
		}

		GS_CORE_ASSERT(0);
		return -1;
	}
}