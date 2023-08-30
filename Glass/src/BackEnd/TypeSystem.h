#pragma once

#include "Metadata.h"
#include "Base/Assert.h"

namespace Glass {

	enum class TypeStorageKind : u32 {
		Base,
		Pointer,

		Array,
		StaticArray,
		DynArray,

		Function,
	};

	//Basic-Types-----------------------------------------//////////////////
	struct TypeStorage {
		u64 Hash = -1;
		u32 BaseID = -1;
		TypeStorageKind Kind;
	};

	struct TSPtr : public TypeStorage {
		TypeStorage* Pointee;
		u16 Indirection = -1;
	};

	struct TSArray : public TypeStorage {
	};

	struct TSDynArray : public TypeStorage {
		TypeStorage* ElementType = nullptr;
	};

	struct TSStaticArray : public TypeStorage {
		u32 Size = -1;
	};
	//////////////////////////////////////////////////////////////////////////
	//Composite-Types-----------------------------------------//////////////////
	struct TSFunc : public TypeStorage {
		TypeStorage* ReturnType = nullptr;
		std::vector<TypeStorage*> Arguments;
	};
	//////////////////////////////////////////////////////////////////////////

	inline u64 BasicTypeHash(u32 type_id) {
		return std::hash<u32>{}(type_id);
	}

	inline u64 PtrHash(u64 pointee_hash, u16 indirection) {
		return Combine2Hashes(pointee_hash, std::hash<u16>{}(indirection));
	}

	inline u64 DynArrayHash(u64 element_hash) {
		return Combine2Hashes(element_hash, std::hash<std::string>{}("dyn array hash"));
	}

	inline u64 FunctionArgumentsHash(const std::vector<TypeStorage*>& argument_hashes, TypeStorage* return_type_hash) {
		u64 hash = std::hash<std::string>{}("func type hash base");

		for (auto argument_hash : argument_hashes) {
			GS_CORE_ASSERT(argument_hash);
			hash = Combine2Hashes(hash, argument_hash->Hash);
		}

		if (return_type_hash) {
			return Combine2Hashes(hash, return_type_hash->Hash);
		}
		else {
			return hash;
		}
	}

	struct TypeSystem {

		TypeSystem(const MetaData& metadata)
			:m_Metadata(metadata)
		{
		}

		static void Init(const MetaData& metadata) {
			GS_CORE_ASSERT(!m_Instance);
			m_Instance = new TypeSystem(metadata);
		}

		static TypeStorage* GetBasic(const std::string& type_name) {

			u64 type_id = m_Instance->m_Metadata.GetType(type_name);
			if (type_id == -1) {
				return nullptr;
			}

			return GetBasic(type_id);
		}

		static TSPtr* GetPtr(TypeStorage* pointee, u32 indirection) {
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

		static TSDynArray* GetDynArray(TypeStorage* element) {

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

		static TypeStorage* GetBasic(u64 type_id) {
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

		static TypeStorage* GetFunction(const std::vector<TypeStorage*>& arguments, TypeStorage* return_type) {

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

		static TypeStorage* IncreaseIndirection(TypeStorage* type) {
			if (type->Kind == TypeStorageKind::Pointer) {
				return TypeSystem::GetPtr(((TSPtr*)type)->Pointee, ((TSPtr*)type)->Indirection + 1);
			}
			else {
				return TypeSystem::GetPtr(type, 1);
			}
		}

		static TypeStorage* ReduceIndirection(TSPtr* pointer) {
			if (pointer->Indirection - 1 != 0) {
				return TypeSystem::GetPtr(pointer->Pointee, pointer->Indirection - 1);
			}
			else {
				return pointer->Pointee;
			}
		}

		static u16 IndirectionCount(TypeStorage* type) {
			if (type->Kind != TypeStorageKind::Pointer) {
				return 0;
			}
			else {
				return ((TSPtr*)type)->Indirection;
			}
		}

		static bool IsPointer(TypeStorage* type) {
			return type->Kind == TypeStorageKind::Pointer;
		}

		static bool IsArray(TypeStorage* type) {
			return type->Kind == TypeStorageKind::Array;
		}

		static std::unordered_map<u64, TypeStorage*>& GetTypeMap() {
			return m_Instance->m_Types;
		}

		static u64 GetTypeInfoIndex(TypeStorage* ts) {
			return std::distance(m_Instance->m_Types.begin(), m_Instance->m_Types.find(ts->Hash));
		}

	private:

		const MetaData& m_Metadata;

		std::unordered_map<u64, TypeStorage*> m_Types;

		static TypeSystem* m_Instance;
	};
}