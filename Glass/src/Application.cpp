#include "pch.h"
#include "Application.h"

#include "StrUtils.h"

#include "FrontEnd/Lexer.h"
#include "FrontEnd/Parser.h"

#include "BackEnd/Compiler.h"
#include "BackEnd/C/CTranspiler.h"

#include "BackEnd/LLVMBackend.h"

namespace Glass
{
	Application::Application(const CommandLineArgs& CmdLineArgs)
		: m_Arguments(CmdLineArgs), m_Options(ParseOptions(CmdLineArgs))
	{

		GS_CORE_WARN("C Includes");

		for (const auto& include : m_Options.CIncludes) {
			GS_CORE_INFO("\t{}", include);
		}

		GS_CORE_WARN("C Libs");

		for (const auto& lib : m_Options.CLibs) {
			GS_CORE_INFO("\t{}", lib);
		}

		Init();
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

			for (CompilerFile& comp_file : compiler_files) {
				m_Sources[comp_file.GetID()] = comp_file;
			}
		}

		{
			m_LexerStart = std::chrono::high_resolution_clock::now();
			for (auto& [id, comp_file] : m_Sources)
			{
				Lexer lexer(comp_file.GetSource(), comp_file.GetPath());

				comp_file.SetTokens(lexer.Lex());
			}
			m_LexerEnd = std::chrono::high_resolution_clock::now();
		}
		{
			m_ParserStart = std::chrono::high_resolution_clock::now();
			for (auto& [id, comp_file] : m_Sources)
			{
				Parser parser(comp_file);
				auto ast = parser.CreateAST();

				comp_file.SetAST(ast);
			}
			m_ParserEnd = std::chrono::high_resolution_clock::now();
		}

		std::vector<CompilerFile*> compiler_files;

		for (auto& [id, comp_file] : m_Sources)
		{
			compiler_files.push_back(&comp_file);
		}

		Compiler compiler(compiler_files);
		IRTranslationUnit* code;

		bool compilation_successful = true;

		{
			m_CompilerStart = std::chrono::high_resolution_clock::now();
			code = compiler.CodeGen();
			m_CompilerEnd = std::chrono::high_resolution_clock::now();

			if (m_Options.DumpIR)
			{
				for (const auto inst : code->Instructions) {
					std::string inst_string = inst->ToString();
					GS_CORE_INFO("\n" + inst_string);
				}
			}

			for (const CompilerMessage& msg : compiler.GetMessages()) {

				MessageType Type = msg.Type;

				if (Type == MessageType::Error) {
					compilation_successful = false;
				}

				switch (Type)
				{
				case MessageType::Info:
					GS_CORE_INFO(msg.message);
					break;
				case MessageType::Warning:
					GS_CORE_WARN(msg.message);
					break;
				case MessageType::Error:
					GS_CORE_ERROR(msg.message);
					break;
				default:
					break;
				}
			}
		}

		if (!m_Options.DumpIR) {
			GS_CORE_INFO("IR Generation Done");
		}

		bool llvm = true;

		if (llvm)
		{
			LLVMBackend llvm_backend = LLVMBackend(&compiler.GetMetadata(), code);

			llvm_backend.Compile();

			std::string libraries;

			for (auto& library : m_Options.CLibs) {
				libraries.push_back(' ');
				libraries += library;
			}

			std::string linker_cmd;

			std::string libraries_cmd;

			for (auto& library : libraries) {
				libraries_cmd += "-l" + library;
			}

			std::string input_name = "output.obj";
			std::string exe_name = "a.exe";

			//linker_cmd = fmt::format("ld.exe {} -lmsvcrt {} -o a.exe", input_name, libraries);
			linker_cmd = fmt::format("clang.exe -g {} {} -o a.exe", input_name, libraries);

			GS_CORE_WARN("Running: {}", linker_cmd);

			int lnk_result = system(linker_cmd.c_str());

			if (lnk_result != 0) {
				GS_CORE_ERROR("Error: During Execution of Command");
			}
			else {
				GS_CORE_WARN("CodeGen Done: {}", linker_cmd);
				if (m_Options.Run) {

					GS_CORE_INFO("Running: {}", exe_name);
					int run_result = system(exe_name.c_str());

					if (run_result != 0) {
						GS_CORE_ERROR("Execution Of Program Existed With Code: {}", run_result);
					}
				}
			}

			GS_CORE_WARN("Lines Processed: {}", g_LinesProcessed);
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

				if (arg == "-ir") {
					options.DumpIR = true;
				}

				continue;
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
}