#pragma once

#include "BackEnd/Il.h"

namespace Glass
{
	struct X64_Gen_Spec
	{
		String output_path;
	};

	struct X64_Gen
	{
		String output_path;
		Il_Program* prog;
	};

	X64_Gen X86_Gen_Make(X64_Gen_Spec spec, Il_Program* program)
	{
		X64_Gen g = { 0 };
		g.output_path = String_Copy(spec.output_path);
		g.prog = program;

		return g;
	}
}