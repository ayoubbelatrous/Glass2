#pragma once

#include "Metadata.h"
#include "Base/Assert.h"

namespace Glass {

	enum class TypeStorageKind : u32 {
		Base,
		Pointer,

		StaticArray,
		DynArray,

		Function,

		Poly,
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

		TypeSystem(const MetaData& metadata);

		static void Init(const MetaData& metadata);

		static TypeFlags GetTypeFlags(TypeStorage* type);

		static TypeStorage* GetBasic(const std::string& type_name);
		static TSPtr* GetPtr(TypeStorage* pointee, u32 indirection);
		static TSDynArray* GetDynArray(TypeStorage* element);
		static TypeStorage* GetBasic(u64 type_id);
		static TypeStorage* GetFunction(const std::vector<TypeStorage*>& arguments, TypeStorage* return_type);

		static TypeStorage* IncreaseIndirection(TypeStorage* type);
		static TypeStorage* ReduceIndirection(TSPtr* pointer);
		static TypeStorage* ReduceIndirection(TSPtr* pointer, u16 level);
		static u16 IndirectionCount(TypeStorage* type);

		static bool IsFlt(TypeStorage* type);
		static bool IsUnSigned(TypeStorage* type);

		static bool IsPointer(TypeStorage* type);
		static bool IsArray(TypeStorage* type);
		static TypeStorage* GetArrayElementTy(TypeStorage* type);

		static std::vector<TypeStorage*>& GetTypeMap();
		static u64 GetTypeInfoIndex(TypeStorage* ts);

		static bool StrictPromotion(TypeStorage* A, TypeStorage* B);

		static TypeStorage* TypeExpressionGetType(TypeExpression* type_expr);

		static u64 GetTypeSize(TypeStorage* type);

		static TypeStorage* GetVoid();

		static TypeStorage* GetBool();

		static TypeStorage* GetU64();
		static TypeStorage* GetU32();
		static TypeStorage* GetU8();

		static TypeStorage* GetI64();
		static TypeStorage* GetI32();
		static TypeStorage* GetI8();

		static TypeStorage* GetVoidPtr();

		static TypeStorage* GetAny();
		static TypeStorage* GetType();
		static TypeStorage* GetArray();

		static TypeStorage* GetString();

		static std::string PrintType(TypeStorage* type);
		static std::string PrintTypeNoSpecialCharacters(TypeStorage* type);

	private:

		static void Insert(u64 hash, TypeStorage* type) {
			m_Instance->m_Types[hash] = m_Instance->m_OrderedTypeArray.size();
			m_Instance->m_OrderedTypeArray.push_back(type);
		}

		static TypeStorage* Get(u64 hash) {
			return m_Instance->m_OrderedTypeArray[m_Instance->m_Types.at(hash)];
		}

		const MetaData& m_Metadata;

		std::unordered_map<u64, u64> m_Types;
		std::vector<TypeStorage*> m_OrderedTypeArray;

		static TypeSystem* m_Instance;
	};
}