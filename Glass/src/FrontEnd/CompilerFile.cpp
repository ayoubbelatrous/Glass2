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
		for (const Token& tk : tokens) {
			m_Tokens.push_back(tk);
		}
	}

	const Token& CompilerFile::GetToken(u64 id) const
	{
		return m_Tokens[id];
	}

	const std::vector<Glass::Token>& CompilerFile::GetTokens() const
	{
		return m_Tokens;
	}

	void CompilerFile::SetAST(ModuleFile* ast) {
		m_Ast = ast;
	}

	ModuleFile* CompilerFile::GetAST() {
		return m_Ast;
	}

	CompilerFile* CompilerFile::GenerateCompilerFile(const fs_path& path)
	{
		std::string source;

		std::ifstream in(path);
		std::stringstream buffer;
		buffer << in.rdbuf();
		source = buffer.str();

		CompilerFile* compilerFile = new CompilerFile(-1, source, path);

		return compilerFile;
	}
}