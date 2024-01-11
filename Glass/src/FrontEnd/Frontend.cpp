#include "pch.h"

#include "FrontEnd/Frontend.h"

namespace Glass
{
	Front_End::Front_End(ApplicationOptions options)
		: Options(options)
	{
	}

	void Front_End::Compile()
	{
		LoadFirst();
	}

	void Front_End::LoadFirst()
	{
		if (Options.Files.size() == 0) {
			Push_Error("you need to provide a start file");
		}

		if (Options.Files.size() > 1) {
			Push_Error("you only need to provide 1 start file");
		}
	}

	void Front_End::Push_Error(const std::string& error)
	{
		Data.Messages.push_back(Front_End_Message{ error, Message_Error });
	}
}