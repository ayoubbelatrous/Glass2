#include "pch.h"

#include "BackEnd/TypeSystem.h"
#include "BackEnd/Metadata.h"

#include "BackEnd/TypeSystem.h"

namespace Glass {

	void MetaData::RegisterFunction(u64 ID, const FunctionMetadata& metadata)
	{
		GS_CORE_ASSERT(m_Functions.find(ID) == m_Functions.end(), "function already was inserted");

		m_Functions[ID] = metadata;
		m_FunctionNames[metadata.Symbol.Symbol] = ID;
	}

	void MetaData::RegisterType(u64 ID, const std::string& name, u64 size, u64 alignment)
	{
		m_Types[ID] = name;
		m_TypeNames[name] = ID;
		m_TypeSizes[ID] = size;
		m_TypeFlags[ID] = 0;
		m_TypeAlignments[ID] = alignment;
		TypeSystem::GetBasic(ID);
	}

	const u64 MetaData::GetTypeSize(TypeStorage* type) const
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
			return GetTypeSize(type->BaseID);
		}

		GS_CORE_ASSERT(0);
		return -1;
	}

	const u64 MetaData::GetTypeAlignment(TypeStorage* type) const
	{
		if (type->Kind == TypeStorageKind::Pointer) {
			return 8;
		}

		if (type->Kind == TypeStorageKind::DynArray) {
			return 8;
		}

		if (type->Kind == TypeStorageKind::Function) {
			return 8;
		}

		if (type->Kind == TypeStorageKind::Base) {
			return GetTypeAlignment(type->BaseID);
		}

		GS_CORE_ASSERT(0);
		return -1;
	}

	void MetaData::ComputeStructSizeAlignOffsets(StructMetadata* metadata)
	{
		u64 size = 0;
		u64 alignment = 1;

		for (const MemberMetadata& member : metadata->Members) {

			auto member_alignment = GetTypeAlignment(member.Type);

			GS_CORE_ASSERT(member_alignment != -1);

			if (member_alignment > alignment) {
				alignment = member_alignment;
			}
		}

		i64 offset = 0;
		// 
		// 		if (metadata->Name.Symbol == "SDL_Keysym") {
		// 			__debugbreak();
		// 		}

		for (MemberMetadata& member : metadata->Members) {
			auto member_size = (i64)GetTypeSize(member.Type);
			auto type_alignment = (i64)GetTypeAlignment(member.Type);

			member.Offset = offset;

			if ((offset + member_size) % type_alignment != 0) {
				i64 padding = (type_alignment - ((offset + member_size) % type_alignment)) % type_alignment;
				offset += padding;
				member.Offset = offset;
				offset += member_size;
			}
			else {
				offset += member_size;
			}

		}

		size = ((alignment - (offset % alignment)) % alignment) + offset;

		if (size != 0 && alignment != 0) {
			size_t finalPadding = (alignment - (size % alignment)) % alignment;
			size += finalPadding;

			GS_CORE_ASSERT(size % alignment == 0);

		}

		metadata->Size = size;
		metadata->Alignment = alignment;
	}

	u64 MetaData::ComputeStructSize(const StructMetadata* metadata)
	{
		u64 size = 0;
		u64 alignment = 0;

		for (const MemberMetadata& member : metadata->Members) {

			auto member_alignment = GetTypeAlignment(member.Type);

			GS_CORE_ASSERT(member_alignment);
			GS_CORE_ASSERT(member_alignment != -1);

			if (member_alignment > alignment) {
				alignment = member_alignment;
			}
		}

		i64 remainder = 0;

		for (const MemberMetadata& member : metadata->Members) {
			auto member_size = (i64)GetTypeSize(member.Type);

			if (member_size >= alignment) {
				remainder = member_size % alignment;
				size += member_size;
			}
			else {
				if (remainder - member_size >= 0) {
					size += remainder = remainder - member_size;
				}
				else {
					remainder = member_size % alignment;
					size += (member_size % alignment) + remainder;
				}
			}
		}

		if (size == 0) {
			return 0;
		}

		size_t finalPadding = (alignment - (size % alignment)) % alignment;
		size += finalPadding;

		//GS_CORE_WARN("struct {} size:{}, aligned:{}", metadata->Name.Symbol, size, alignment);
		GS_CORE_ASSERT(size % alignment == 0);

		return size;
	}

	SymbolType MetaData::GetSymbolType(const std::string& symbol) const
	{
		if (GetFunctionMetadata(symbol) != (u64)-1) {
			return SymbolType::Function;
		}

		if (GetGlobalVariable(symbol) != (u64)-1) {
			return SymbolType::GlobVariable;
		}

		if (GetVariableMetadata(GetVariableRegister(symbol)) != nullptr) {
			return SymbolType::Variable;
		}

		if (GetEnum(symbol) != nullptr) {
			return SymbolType::Enum;
		}

		if (GetType(symbol) != (u64)-1) {
			return SymbolType::Type;
		}

		if (GetConstant(symbol) != nullptr) {
			return SymbolType::Constant;
		}

		if (GetLibrary(symbol) != nullptr) {
			return SymbolType::Library;
		}

		return SymbolType::None;
	}

	IRFunction* FunctionMetadata::FindPolymorphicOverload(const PolymorphicOverload& query)
	{
		for (auto& [overload, func] : PolyMorphicInstantiations) {

			bool equal = true;

			if (overload.Arguments.size() != query.Arguments.size()) {
				equal = false;
			}

			for (auto& [name, value] : overload.Arguments)
			{
				auto this_type = TypeSystem::TypeExpressionGetType((TypeExpression*)value);
				auto other_type = TypeSystem::TypeExpressionGetType((TypeExpression*)query.Arguments.at(name));

				if (this_type != other_type) {
					equal = false;
				}
			}

			if (equal) {
				return func;
			}
			else {
				continue;
			}
		}

		return nullptr;
	}

	bool FunctionMetadata::IsOverloaded() const
	{
		return Overloads.size() != 0;
	}

	void FunctionMetadata::AddOverload(const FunctionMetadata& function)
	{
		GS_CORE_ASSERT(function.Signature, "function.Signature is null");
		Overloads[(TSFunc*)function.Signature] = function;

		auto arguments_only_type = (TSFunc*)TypeSystem::GetFunction(((TSFunc*)function.Signature)->Arguments, TypeSystem::GetVoid());
		GS_CORE_ASSERT(arguments_only_type, "arguments_only_type is null");

		OverloadsArgLookUp[arguments_only_type] = &Overloads[(TSFunc*)function.Signature];
	}

	FunctionMetadata* FunctionMetadata::FindOverload(TSFunc* signature)
	{
		GS_CORE_ASSERT(signature);

		auto it = Overloads.find(signature);

		if (it == Overloads.end()) {
			return nullptr;
		}
		else {
			return &it->second;
		}
	}

	FunctionMetadata* FunctionMetadata::FindOverloadForCall(TSFunc* signature)
	{
		GS_CORE_ASSERT(signature);

		auto it = OverloadsArgLookUp.find(signature);

		if (it == OverloadsArgLookUp.end()) {
			return nullptr;
		}
		else {
			return it->second;
		}
	}

	FunctionMetadata& FunctionMetadata::GetOverload(TSFunc* signature)
	{
		GS_CORE_ASSERT(signature, "signature is null");
		return Overloads.at(signature);
	}

	const FunctionMetadata& FunctionMetadata::GetOverload(TSFunc* signature) const
	{
		GS_CORE_ASSERT(signature);
		return Overloads.at(signature);
	}

	const ArgumentMetadata* FunctionMetadata::GetArgument(u64 i) const
	{
		if (i > Arguments.size() - 1) {
			return nullptr;
		}
		else {
			return &Arguments[i];
		}
	}

	bool PolymorphicOverload::operator<(const PolymorphicOverload& other) const
	{
		if (this->Arguments.size() != other.Arguments.size()) {
			return true;
		}

		for (auto& [name, value] : this->Arguments)
		{
			auto this_type = TypeSystem::TypeExpressionGetType((TypeExpression*)value);
			auto other_type = TypeSystem::TypeExpressionGetType((TypeExpression*)other.Arguments.at(name));

			if (this_type != other_type) {
				return true;
			}
		}

		return false;
	}
}