#include "pch.h"

#include "FrontEnd/Frontend.h"
#include "FrontEnd/Lexer.h"
#include "FrontEnd/Parser.h"

namespace Glass
{
	Front_End::Front_End(ApplicationOptions options)
		: Options(options)
	{
	}

	void Front_End::Compile()
	{
		Load_First();
	}

	void Front_End::Load_First()
	{
		if (Options.Files.size() == 0) {
			Push_Error("you need to provide a start file");
		}

		if (Options.Files.size() > 1) {
			Push_Error("you only need to provide 1 start file");
		}

		fs_path first_file_path_abs = std::filesystem::absolute(Options.Files[0]);
		File_ID first_file_id = Generate_File(Options.Files[0], first_file_path_abs);
	}

	void Front_End::Push_Error(const std::string& error)
	{
		Data.Messages.push_back(Front_End_Message{ error, Message_Error });
	}

	File_ID Front_End::Generate_File(const fs_path& path, const fs_path& absolute_path)
	{
		Front_End_File file;
		file.Path = path;
		file.Absolute_Path = absolute_path;

		std::string source;

		{
			std::ifstream in(file.Path);
			std::stringstream buffer;
			buffer << in.rdbuf();
			source = buffer.str();
		}

		Lexer lexer = Lexer(source, file.Path);
		file.Tokens = lexer.Lex();

		Parser parser = Parser(file.Path, file.Tokens);
		file.Syntax = parser.CreateAST();

		File_ID file_identifier = Data.Files.size();

		Data.Files.push_back(std::move(file));
		Data.Path_To_File[file.Path.string()] = file_identifier;

		return file_identifier;
	}
}