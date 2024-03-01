#pragma once

#include "Base/String.h"
#include "Base/Array.h"

namespace Glass
{
	struct String_Atom
	{
		String str;
		u64 hash;
	};

	struct String_Atom_Table
	{
		std::unordered_map<std::string_view, String_Atom*> atom_table;
		Array<String_Atom> atoms;
	};

	String_Atom* get_atom(const String& str);
	String_Atom* get_atom(const char* str);
	String_Atom* get_atom(const std::string& str);
}
