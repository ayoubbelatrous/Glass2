#pragma once

#include "BackEnd/Compiler.h"

namespace Glass
{
	struct Interpeter {
		IRInstruction* Evaluate_Expression(std::vector<IRInstruction*> instructions);
	};
}