#include "pch.h"

#include "BackEnd/TypeSystem.h"
#include "BackEnd/Metadata.h"

namespace Glass {
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