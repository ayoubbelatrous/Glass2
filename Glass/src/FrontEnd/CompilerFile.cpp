#include "pch.h"

#include "FrontEnd/CompilerFile.h"
#include "FrontEnd/Ast.h"

namespace Glass {

	CompilerFile::CompilerFile(u64 id, const std::string& source, const fs_path& path)
		: m_ID(id), m_Source(source), m_Path(path)
	{}

	u64 CompilerFile::GetID() const
	{
		return m_ID;
	}

	const std::string& CompilerFile::GetSource() const
	{
		return m_Source;
	}

	const fs_path& CompilerFile::GetPath() const
	{
		return m_Path;
	}

	void CompilerFile::SetTokens(const std::vector<Token>& tokens)
	{
		u64 token_counter = 0;

		for (const Token& tk : tokens) {
			token_counter++;
			m_TokenIDs.push_back(token_counter);
			m_Tokens.emplace(token_counter, tk);
		}
	}

	const Token& CompilerFile::GetToken(u64 id) const
	{
		return m_Tokens.at(id);
	}

	const std::vector<u64>& CompilerFile::GetTokens() const
	{
		return m_TokenIDs;
	}

	void CompilerFile::SetAST(ModuleFile* ast) {
		m_Ast = ast;
	}

	ModuleFile* CompilerFile::GetAST() {
		return m_Ast;
	}
}