#include "pch.h"

#include "Application.h"

#include "StrUtils.h"
#include "FrontEnd/Lexer.h"
#include "FrontEnd/Parser.h"
#include "FrontEnd/Frontend.h"

namespace Glass
{
	Application::Application(const CommandLineArgs& CmdLineArgs)
		: m_Arguments(CmdLineArgs), m_Options(ParseOptions(CmdLineArgs))
	{
		Init();
	}

	void Application::OnStart()
	{
		if (m_Options.Files.empty()) {
			FatalAbort(ExitCode::InvalidCommandLineInput, "No Input Source Files!");
		}

		Front_End front_end = Front_End(m_Options);
		front_end.Compile();

		for (size_t i = 0; i < front_end.Data.Messages.count; i++)
		{
			Front_End_Message& message = front_end.Data.Messages[i];

			switch (message.Message_Type)
			{
			case Message_Error:
				GS_CORE_ERROR("{}", message.Message);
				break;
			case Message_Warning:
				GS_CORE_WARN("{}", message.Message);
				break;
			case Message_Info:
				GS_CORE_INFO("{}", message.Message);
				break;
			default:
				GS_CORE_ASSERT(nullptr, "un reachable!");
				break;
			}
		}
	}

	void Application::OnShutdown()
	{

	}

	void Application::Init()
	{
	}

	const CompilerFile& Application::GetCompilerFile(u64 id) const
	{
		GS_CORE_ASSERT(m_Sources.find(id) != m_Sources.end(), "Compilation File must exist");
		return m_Sources.at(id);
	}

	const std::map<u64, CompilerFile>& Application::GetCompilerFiles() const
	{
		return m_Sources;
	}

	void Application::Abort(ExitCode code, const std::string& message)
	{
		if ((int)code < 0)
		{
			GS_CORE_WARN("Abnormal Abort: {} With Message: {}", (int)code, message);
		}

		exit((int)code);
	}

	void Application::FatalAbort(ExitCode code, const std::string& message)
	{
		GS_CORE_ERROR("Fatal Abort: {} With Message: {}", (int)code, message);
		exit((int)code);
	}

	ApplicationOptions Application::ParseOptions(const CommandLineArgs& CmdLineArgs)
	{
		bool modeOutput = false;
		bool modeCIncludes = false;
		bool modeCLibs = false;
		bool modeAssemblerSelect = false;

		bool modal = false;

		ApplicationOptions options;

		for (const std::string& arg : CmdLineArgs.Arguments) {

			if (FindStringIC(arg, "-")) {

				modeOutput = false;
				modeCIncludes = false;
				modeCLibs = false;

				modal = true;

				if (arg == "-o") {
					modeOutput = true;
				}

				if (arg == "-cI") {
					modeCIncludes = true;
				}

				if (arg == "-cL") {
					modeCLibs = true;
				}

				if (arg == "-run") {
					options.Run = true;
					modal = false;
				}

				if (arg == "-il") {
					options.Backend = Backend_Option::Il_Backend;
					modal = false;
				}

				if (arg == "-llvm") {
					options.Backend = Backend_Option::LLVM_Backend;
					modal = false;
				}

				if (arg == "-S") {
					options.Dissassemble = 1;
					modal = false;
				}

				if (arg == "-dll") {
					options.OutputDll = true;
					modal = false;
				}

				if (arg == "-ir") {
					options.DumpIR = true;
					modal = false;
				}

				if (arg == "-no-link") {
					options.NoLink = true;
					modal = false;
				}

				if (arg == "-asm") {
					modeAssemblerSelect = true;
				}

				continue;
			}

			if (modal) {

				if (modeOutput) {
					if (FindStringIC(arg, "-")) {
						FatalAbort(ExitCode::InvalidCommandLineInput, "Expected A Valid Output Path After -o");
					}
					else {
						options.Output = arg;
						modeOutput = false;
					}
				}

				if (modeAssemblerSelect) {
					if (FindStringIC(arg, "-")) {
						FatalAbort(ExitCode::InvalidCommandLineInput, "Expected A Valid Assembler Name After -asm options are: fasm, clang");
					}
					else {

						modeAssemblerSelect = false;
					}
				}

				if (modeCIncludes) {
					options.CIncludes.push_back(arg);
				}

				if (modeCLibs) {
					options.CLibs.push_back(arg);
				}
			}
			else {

				if (!FindStringIC(arg, "-")) {
					if (std::filesystem::exists(arg)) {
						options.Files.push_back(arg);
					}
					else {
						GS_CORE_ERROR("File Was Not Found: {}!", arg);
					}
				}
			}
		}

		if (options.Output.empty()) {
			options.Output = "./app.exe";
		}

		return options;
	}

	bool Application::UseAlloca = true;

	std::vector<CompilerFile> Application::GenerateCompilationFiles(const std::vector<fs_path>& files)
	{
		std::vector<CompilerFile> compilerFiles;

		u64 IDCounter = 1;

		for (const fs_path& file : files)
		{
			std::string source;

			{
				std::ifstream in(file);
				std::stringstream buffer;
				buffer << in.rdbuf();
				source = buffer.str();
			}

			CompilerFile compilerFile = CompilerFile(IDCounter, source, file);

			compilerFiles.push_back(compilerFile);

			IDCounter++;
		}

		return compilerFiles;
	}

	const u64 allocator_buffer_size = (1024 * 1024) * 30;

	LinearAllocator Application::m_AstAllocator = LinearAllocator(allocator_buffer_size);
	LinearAllocator Application::m_IRAllocator = LinearAllocator(allocator_buffer_size);
	LinearAllocator Application::m_TypeAllocator = LinearAllocator(allocator_buffer_size);
	LinearAllocator Application::m_ASMAllocator = LinearAllocator(allocator_buffer_size);
}