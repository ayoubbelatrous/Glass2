#pragma once

#include "FrontEnd/Token.h"

namespace Glass
{
	class Lexer
	{
	public:
		Lexer(const std::string& source, const fs_path& file_path);

		std::vector<Token> Lex();

	private:
		const std::string& m_Source;
		const fs_path& m_Path;
	};
}