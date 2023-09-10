#pragma once

#include "FrontEnd/Token.h"

namespace Glass
{
	class ModuleFile;

	class CompilerFile
	{
	public:
		CompilerFile() = default;
		CompilerFile(u64 id, const std::string& source, const fs_path& path);

		u64 GetID() const;
		const std::string& GetSource() const;
		const fs_path& GetPath() const;

		void SetTokens(const std::vector<Token>& tokens);
		const Token& GetToken(u64 id) const;
		const std::vector<Token>& GetTokens() const;

		void SetAST(ModuleFile* ast);

		ModuleFile* GetAST();

		static CompilerFile* GenerateCompilerFile(const fs_path& path);

	private:
		u64 m_ID = 0;
		std::string m_Source = "";
		fs_path m_Path = "";

		std::vector<Token> m_Tokens;

		ModuleFile* m_Ast = nullptr;
	};
}