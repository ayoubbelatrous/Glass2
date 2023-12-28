#include "pch.h"
#include "Application.h"

#include "StrUtils.h"

#include "FrontEnd/Lexer.h"
#include "FrontEnd/Parser.h"

#include "BackEnd/Compiler.h"
#include "BackEnd/C/CTranspiler.h"

#include "BackEnd/LLVMBackend.h"

#include "BackEnd/LLR_X86.h"

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

		bool llvm = false;

		std::chrono::steady_clock::time_point m_LLVMStart;
		std::chrono::steady_clock::time_point m_LLVMEnd;

		std::chrono::steady_clock::time_point m_X86Start;
		std::chrono::steady_clock::time_point m_X86End;

		std::chrono::steady_clock::time_point m_LinkerStart;
		std::chrono::steady_clock::time_point m_LinkerEnd;

		X86_BackEnd x86_backend = X86_BackEnd(code, &compiler.GetMetadataNonConst());
		if (compilation_successful)
		{
			m_X86Start = std::chrono::high_resolution_clock::now();
			x86_backend.Assemble();
			m_X86End = std::chrono::high_resolution_clock::now();
		}

		if (llvm && compilation_successful)
		{
			LLVMBackend llvm_backend = LLVMBackend(&compiler.GetMetadata(), code);

			m_LLVMStart = std::chrono::high_resolution_clock::now();
			llvm_backend.Compile();
			m_LLVMEnd = std::chrono::high_resolution_clock::now();

			std::string libraries_cmd;

			for (auto& library : m_Options.CLibs) {

				auto lib_as_path = fs_path(library);

				if (lib_as_path.extension().empty()) {
					libraries_cmd += "-l" + library + ' ';
				}
				else {
					libraries_cmd += library + ' ';
				}
			}

			std::string input_name = "output.obj";
			std::string exe_name = "a.exe";
			std::string output_type = "";
			std::string output_name = "a.exe";

			std::string linker_cmd;

			if (m_Options.OutputDll) {
				output_type = "-shared";
				output_name = "a.dll";
			}

			//linker_cmd = fmt::format("ld.exe {} -lmsvcrt {} -o a.exe", input_name, libraries);
			linker_cmd = fmt::format("clang.exe -g {} {} {} -o {}", output_type, input_name, libraries_cmd, output_name);

			if (!m_Options.NoLink && llvm)
			{
				GS_CORE_WARN("Running: {}", linker_cmd);

				m_LinkerStart = std::chrono::high_resolution_clock::now();
				int lnk_result = system(linker_cmd.c_str());
				m_LinkerEnd = std::chrono::high_resolution_clock::now();

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
			}

			GS_CORE_WARN("Lines Processed: {}", g_LinesProcessed);
			GS_CORE_WARN("Lexer Took: {} micro", std::chrono::duration_cast<std::chrono::microseconds>(m_LexerEnd - m_LexerStart).count());
			GS_CORE_WARN("Parser Took: {} micro", std::chrono::duration_cast<std::chrono::microseconds>(m_ParserEnd - m_ParserStart).count());
			GS_CORE_WARN("LLVM Took: {} ms", std::chrono::duration_cast<std::chrono::milliseconds>(m_LLVMEnd - m_LLVMStart).count());
			GS_CORE_WARN("Linker Took: {} ms", std::chrono::duration_cast<std::chrono::milliseconds>(m_LinkerEnd - m_LinkerStart).count());
			GS_CORE_WARN("IR Generation Took: {} ms", std::chrono::duration_cast<std::chrono::milliseconds>(m_CompilerEnd - m_CompilerStart).count());
			GS_CORE_WARN("X86_64 Backend Took: {} micro s", std::chrono::duration_cast<std::chrono::microseconds>(m_X86End - m_X86Start).count());
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
	LinearAllocator Application::m_ASMAllocator = LinearAllocator(allocator_buffer_size);
}