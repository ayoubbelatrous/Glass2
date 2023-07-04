#include "pch.h"
#include "Application.h"

#include "StrUtils.h"

#include "FrontEnd/Lexer.h"

namespace Glass
{
	Application::Application(const CommandLineArgs& CmdLineArgs)
		: m_Arguments(CmdLineArgs), m_Options(ParseOptions(CmdLineArgs))
	{
	}

	void Application::OnStart()
	{
		if (m_Options.Files.empty()) {
			FatalAbort(ExitCode::InvalidCommandLineInput, "No Input Source Files!");
		}

		if (m_Options.Verbose) {
			for (auto& file : m_Options.Files) {
				GS_CORE_INFO("Compiling: {}", file);
			}
		}

		{
			auto compiler_files = GenerateCompilationFiles(m_Options.Files);

			for (auto& comp_file : compiler_files) {
				m_Sources[comp_file.GetID()] = comp_file;
			}
		}

		{
			for (auto& [id, comp_file] : m_Sources) {
				Lexer lexer(comp_file.GetSource(), comp_file.GetPath());

				auto tokens = lexer.Lex();

				const fs_path& path = comp_file.GetPath();

				for (auto tk : tokens) {
					GS_CORE_INFO("Token = {0} ; {1}", tk.ToString(), fmt::format("{0}:{1}:{2}", path, tk.Line + 1, tk.Begin));
				}
			}
		}
	}

	void Application::OnShutdown()
	{

	}

	const Glass::CompilerFile& Application::GetCompilerFile(u64 id) const
	{
		GS_CORE_ASSERT(m_Sources.find(id) != m_Sources.end(), "Compilation File must exist");
		return m_Sources.at(id);
	}

	const std::map<u64, Glass::CompilerFile>& Application::GetCompilerFiles() const
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
		bool modal = false;

		ApplicationOptions options;

		for (const std::string& arg : CmdLineArgs.Arguments) {

			if (FindStringIC(arg, "-")) {
				modal = true;
				if (arg == "-o") {
					modeOutput;
				}
			}

			if (modal) {
				if (modeOutput) {
					if (FindStringIC(arg, "-")) {
						FatalAbort(ExitCode::InvalidCommandLineInput, "Expected A Valid Output Path After -o Instead Got Nothing");
					}
					else {
						options.Output = arg;
						modeOutput = false;
					}
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
}