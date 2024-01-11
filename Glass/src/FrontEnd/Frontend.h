#pragma once

#include "Application.h"

namespace Glass
{
	enum Front_End_Message_Type
	{
		Message_Info,
		Message_Warning,
		Message_Error,
	};

	struct Front_End_File {
		fs_path Absolute_Path;
		fs_path Path;
		std::vector<Token> Tokens;
		ModuleFile* Syntax;
	};

	struct Front_End_Message
	{
		std::string Message;
		Front_End_Message_Type Message_Type;
	};

	using File_ID = u64;

	struct Front_End_Data
	{
		std::vector<Front_End_File> Files;
		std::unordered_map<std::string, File_ID> Path_To_File;

		std::vector<Front_End_Message> Messages;
	};

	class Front_End
	{
	public:
		Front_End(ApplicationOptions options);

		void Compile();
		void Load_First();

		void Push_Error(const std::string& error);

		File_ID Generate_File(const fs_path& path, const fs_path& absolute_path);

		Front_End_Data Data;
		ApplicationOptions Options;
	};
}