#pragma once

#include "Base/Types.h"

namespace Glass {
	inline u64 Combine2Hashes(u64 hash1, u64 hash2) {
		hash1 ^= hash2 + 0x9e3779b97f4a7c15 + (hash1 << 6) + (hash1 >> 2);
		return hash1;
	}
}