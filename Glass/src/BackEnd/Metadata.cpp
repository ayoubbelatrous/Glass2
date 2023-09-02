#include "pch.h"

#include "BackEnd/TypeSystem.h"
#include "BackEnd/Metadata.h"

namespace Glass {

	void MetaData::RegisterFunction(u64 ID, const std::string& name, TypeStorage* returnType /*= nullptr*/, std::vector<ArgumentMetadata> args /*= {}*/, bool variadic /*= false*/, const Token& symbol /*= {}*/, TypeStorage* signature /*= nullptr */)
	{
		FunctionMetadata func;
		func.Name = name;
		func.ReturnType = returnType;
		func.Arguments = args;
		func.Variadic = variadic;
		func.Symbol = symbol;

		if (!signature) {
			std::vector<TypeStorage*> signature_arguments;

			for (auto& arg : args) {
				signature_arguments.push_back(arg.Type);
			}

			signature = TypeSystem::GetFunction(signature_arguments, returnType);
		}

		func.Signature = signature;

		m_Functions[ID] = func;
		m_FunctionNames[name] = ID;
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
	}
}