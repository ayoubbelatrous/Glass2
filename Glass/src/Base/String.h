#pragma once

#include "stdlib.h"
#include <cstring>
#include <string>
#include "Base/Types.h"

namespace Glass
{
	struct String;

	String String_Make(char* c_str);
	String String_Make(const std::string& std_str);
	String String_Copy(String other);
	bool String_Equal(String a, String b);

	struct String
	{
		u64 count;
		char* data;

		void operator=(const char* c_str) {
			data = (char*)c_str;
			count = strlen(c_str);
		}

		char& operator[](std::size_t index) {
			if (index >= count) {
				ASSERT("String Out of bounds access");
				return data[count - 1];
			}
			return data[index];
		}

		char operator[](std::size_t index) const {
			if (index >= count) {
				ASSERT("String Out of bounds access");
				return data[count - 1];
			}
			return data[index];
		}
	};
}