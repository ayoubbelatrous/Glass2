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

	struct Front_End_Message
	{
		std::string Message;
		Front_End_Message_Type Message_Type;
	};

	struct Front_End_Data
	{
		std::vector<Front_End_Message> Messages;
	};

	class Front_End
	{
	public:
		Front_End(ApplicationOptions options);

		void Compile();
		void LoadFirst();

		void Push_Error(const std::string& error);

		Front_End_Data Data;
		ApplicationOptions Options;
	};
}