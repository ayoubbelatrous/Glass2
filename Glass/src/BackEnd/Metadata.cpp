#include "pch.h"

#include "BackEnd/TypeSystem.h"
#include "BackEnd/Metadata.h"

namespace Glass {

	void MetaData::RegisterFunction(u64 ID, const FunctionMetadata& metadata)
	{
		GS_CORE_ASSERT(m_Functions.find(ID) == m_Functions.end(), "function already was inserted");

		m_Functions[ID] = metadata;
		m_FunctionNames[metadata.Symbol.Symbol] = ID;
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

}