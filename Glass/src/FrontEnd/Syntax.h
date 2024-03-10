#pragma once

#include "Base/Types.h"
#include "FrontEnd/Atom_Table.h"
#include "FrontEnd/Nodes.h"

namespace Glass
{
	struct Front_End;

	struct Parser_State
	{
		u64			location;
		Array<Tk>	tokens;
		Front_End* f;
		String		file_path;
		bool		error = false;
	};

	void parser_init();

	Ast_Node* copy_statement(Ast_Node* stmt);

	Ast_Node* parse_string(Front_End& f, String file_path, String source, bool& success);
}
