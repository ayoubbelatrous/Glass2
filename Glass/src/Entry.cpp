#include "pch.h"

#include "Application.h"

int main(int argc, char** argv)
{
	Glass::Log::Init();

	std::vector<std::string> commandLineArgs;

	for (size_t i = 1; i < argc; i++)
	{
		commandLineArgs.push_back(argv[i]);
	}

	Glass::Application App = Glass::Application(Glass::CommandLineArgs{ commandLineArgs });

	App.OnStart();
	App.OnShutdown();
}