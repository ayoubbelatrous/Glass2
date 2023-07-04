#include "FrontEnd/CompilerFile.h"

namespace Glass
{
	struct CommandLineArgs {
		std::vector<std::string> Arguments;
	};

	enum class CompilerTargetArch {
		X86_64 = 0,
		X86 = 1
	};

	enum class CompilerTarget {
		C
	};

	struct ApplicationOptions {

		std::vector<fs_path> Files;
		std::string Output;
		CompilerTargetArch TargetArchitecture = CompilerTargetArch::X86_64;
		CompilerTarget CompilerTarget = CompilerTarget::C;

		bool GenerateDebugInfo = false;
		bool Verbose = true;
	};

	enum class ExitCode : int
	{
		Normal = 0,
		InvalidCommandLineInput = -2,
		LexerError = -3,
		ParserError = -4,
		CompilerError = -5,
	};

	class Application
	{
	public:
		Application(const CommandLineArgs& CmdLineArgs);

		void OnStart();
		void OnShutdown();

		const CompilerFile& GetCompilerFile(u64 id) const;
		const std::map<u64, CompilerFile>& GetCompilerFiles() const;

		static void Abort(ExitCode code = ExitCode::Normal, const std::string& message = "");
		static void FatalAbort(ExitCode code = ExitCode::InvalidCommandLineInput, const std::string& message = "");

		static ApplicationOptions ParseOptions(const CommandLineArgs& CmdLineArgs);

	private:

		static std::vector<CompilerFile> GenerateCompilationFiles(const std::vector<fs_path>& files);

		CommandLineArgs m_Arguments;
		ApplicationOptions m_Options;

		std::map<u64, CompilerFile> m_Sources;
	};
}