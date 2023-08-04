#pragma once

#include "BackEnd/Compiler.h"

namespace Glass
{
	class Interpeter
	{
	public:

		Interpeter(IRInstruction* program, const Compiler::MetaData* metadata);

		void Run(u64 Function);

	private:
		const Compiler::MetaData* m_Metadata;
		u64 RegisterPointer = 0;
		std::vector<char> m_Registers;
	};
}