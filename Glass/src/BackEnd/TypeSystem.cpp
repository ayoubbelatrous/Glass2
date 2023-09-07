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
			return (TSDynArray*)it->second;
		}

		TSDynArray* new_type = TYPE(TSDynArray());
		new_type->BaseID = element->BaseID;
		new_type->Kind = TypeStorageKind::DynArray;
		new_type->Hash = hash;
		new_type->ElementType = element;

		m_Instance->m_Types.emplace(hash, new_type);

		return new_type;
	}

	void TypeSystem::Init(const MetaData& metadata)
	{
		GS_CORE_ASSERT(!m_Instance);
		m_Instance = new TypeSystem(metadata);
	}

	TypeStorage* TypeSystem::GetBasic(u64 type_id)
	{
		u64 hash = BasicTypeHash((u32)type_id);

		auto it = m_Instance->m_Types.find(hash);

		if (it != m_Instance->m_Types.end()) {
			return it->second;
		}

		TypeStorage* new_type = TYPE(TypeStorage());
		new_type->BaseID = (u32)type_id;
		new_type->Kind = TypeStorageKind::Base;
		new_type->Hash = hash;

		m_Instance->m_Types.emplace(hash, new_type);

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
			return (TSPtr*)it->second;
		}

		TSPtr* new_type = TYPE(TSPtr());
		new_type->BaseID = pointee->BaseID;
		new_type->Kind = TypeStorageKind::Pointer;
		new_type->Hash = hash;
		new_type->Pointee = pointee;
		new_type->Indirection = indirection;

		m_Instance->m_Types.emplace(hash, new_type);
		return new_type;
	}

	TypeStorage* TypeSystem::GetFunction(const std::vector<TypeStorage*>& arguments, TypeStorage* return_type)
	{
		u64 hash = FunctionArgumentsHash(arguments, return_type);

		auto it = m_Instance->m_Types.find(hash);

		if (it != m_Instance->m_Types.end()) {
			return it->second;
		}

		TSFunc* new_type = TYPE(TSFunc());
		new_type->BaseID = -1;
		new_type->Kind = TypeStorageKind::Function;
		new_type->Hash = hash;
		new_type->Arguments = arguments;
		new_type->ReturnType = return_type;

		m_Instance->m_Types.emplace(hash, new_type);

		return new_type;
	}

	TypeStorage* TypeSystem::GetVoid()
	{
		return GetBasic(IR_void);
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

	std::unordered_map<u64, TypeStorage*>& TypeSystem::GetTypeMap()
	{
		return m_Instance->m_Types;
	}

	u64 TypeSystem::GetTypeInfoIndex(TypeStorage* ts)
	{
		return std::distance(m_Instance->m_Types.begin(), m_Instance->m_Types.find(ts->Hash));
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
}