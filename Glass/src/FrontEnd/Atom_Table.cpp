
#include "pch.h"

#include "FrontEnd/Atom_Table.h"

#include <unordered_set>
#include <string>

namespace Glass
{
	String_Atom_Table string_atom_table;

	String_Atom* get_atom(const String& str)
	{
		auto view = std::string_view{ str.data,str.count };

		auto it = string_atom_table.atom_table.find(view);

		if (it != string_atom_table.atom_table.end())
			return it->second;

		if (!string_atom_table.atoms.data)
		{
			string_atom_table.atoms = Array_Reserved<String_Atom>(0xffff * sizeof(String_Atom));
			string_atom_table.atoms.capacity = 0xffff;
		}

		String_Atom new_atom = {};
		new_atom.hash = std::hash<std::string_view>{}(view);
		new_atom.str = String_Copy(str);

		String_Atom* res = Array_Add(string_atom_table.atoms, new_atom);

		string_atom_table.atom_table[view] = res;

		return res;
	}

	String_Atom* get_atom(const char* str)
	{
		return get_atom(String_Make(str));
	}

	String_Atom* get_atom(const std::string& str)
	{
		return get_atom(str.c_str());
	}
}
