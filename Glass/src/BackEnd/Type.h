#pragma once

#include "Base/Types.h"

namespace Glass {

	enum TypeTypeEnum : u32
	{
		Basic,
		TT_Array,
		Function,
	};

	struct Type {
		u64 ID = 0;
		bool Array = false;
		u64 Pointer = 0;
		TypeTypeEnum TypeType;

		bool operator==(const Type& other) const {
			return ID == other.ID && Array == other.Array && Pointer == other.Pointer && TypeType == other.TypeType;
		}
	};

	struct FunctionType {
		std::vector <Type> Arguments;
		Type ReturnType;
	};
}

namespace std {
	template <>
	struct hash<Glass::Type> {
		std::size_t operator()(const Glass::Type& t) const {
			std::size_t h1 = std::hash<u64>{}(t.ID);
			std::size_t h2 = std::hash<bool>{}(t.Array);
			std::size_t h3 = std::hash<u64>{}(t.Pointer);
			std::size_t h4 = std::hash<u32>{}((u32)t.TypeType);

			return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
		}
	};
}

