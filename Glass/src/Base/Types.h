#pragma once

#include "stdint.h"
#include <filesystem>

typedef int16_t u16;
typedef uint16_t i16;

typedef int64_t i64;
typedef uint64_t u64;

typedef int32_t i32;
typedef uint32_t u32;

namespace Glass {
	using fs_path = std::filesystem::path;
}