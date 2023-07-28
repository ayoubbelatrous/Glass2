#include "pch.h"
#include "Application.h"

#include "StrUtils.h"

#include "FrontEnd/Lexer.h"
#include "FrontEnd/Parser.h"

#include "BackEnd/Compiler.h"
#include "BackEnd/C/CTranspiler.h"

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

				/*
				{
					const fs_path& path = comp_file.GetPath();

					for (auto tk : tokens) {
						GS_CORE_INFO("Token = {0} ; {1}", tk.ToString(), fmt::format("{0}:{1}:{2}", path, tk.Line + 1, tk.Begin));
					}
				}
				*/
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

				//std::string ast_string = ast->ToString();
				//GS_CORE_INFO("\n" + ast_string);
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

			for (const auto inst : code->Instructions) {
				std::string inst_string = inst->ToString();
				GS_CORE_INFO("\n" + inst_string);
			}

			for (const Compiler::CompilerMessage& msg : compiler.GetMessages()) {

				Compiler::MessageType Type = msg.Type;

				if (Type == Compiler::MessageType::Error) {
					compilation_successful = false;
				}

				switch (Type)
				{
				case Compiler::MessageType::Info:
					GS_CORE_INFO(msg.message);
					break;
				case Compiler::MessageType::Warning:
					GS_CORE_WARN(msg.message);
					break;
				case Compiler::MessageType::Error:
					GS_CORE_ERROR(msg.message);
					break;
				default:
					break;
				}
			}
		}

		if (compilation_successful)
		{
			m_TranspilerStart = std::chrono::high_resolution_clock::now();
			CTranspiler transpiler(code, m_Options.CIncludes, &compiler.GetMetadata());
			{
				{
					std::string c_code = transpiler.Codegen();
					m_TranspilerEnd = std::chrono::high_resolution_clock::now();

					std::ofstream out(fs_path(m_Options.Output).remove_filename() / "generated.c");

					out << c_code;
				}

				fs_path output = fs_path(m_Options.Output);
				fs_path input = "generated.c";

				std::string compiler_cmd = fmt::format("clang -w {}", input.string());

				GS_CORE_WARN("Running: {}", compiler_cmd);
				auto clang_start = std::chrono::high_resolution_clock::now();
				int cc_result = system(compiler_cmd.c_str());
				auto clang_end = std::chrono::high_resolution_clock::now();

				if (cc_result != 0) {
					GS_CORE_ERROR("Error: During Execution of Command");
				}

				auto lexer_time = std::chrono::duration_cast<std::chrono::microseconds>(m_LexerEnd - m_LexerStart).count() / 1000.0f;
				auto parser_time = std::chrono::duration_cast<std::chrono::microseconds>(m_ParserEnd - m_ParserStart).count() / 1000.0f;
				auto compiler_time = std::chrono::duration_cast<std::chrono::microseconds>(m_CompilerEnd - m_CompilerStart).count() / 1000.0f;
				auto transpiler_time = std::chrono::duration_cast<std::chrono::microseconds>(m_TranspilerEnd - m_TranspilerStart).count() / 1000.0f;

				if (false) {
					GS_CORE_WARN("Timings: ");
					GS_CORE_WARN("Clang: {}", std::chrono::duration_cast<std::chrono::microseconds>(clang_end - clang_start).count() / 1000.0f);
					GS_CORE_WARN("Total: {}", lexer_time + parser_time + compiler_time + transpiler_time);
					GS_CORE_WARN("Total With Clang: {}", std::chrono::duration_cast<std::chrono::microseconds>(clang_end - m_LexerStart).count() / 1000.0f);
					GS_CORE_WARN("Lexer: {}", lexer_time);
					GS_CORE_WARN("Parser: {}", parser_time);
					GS_CORE_WARN("Compiler: {}", compiler_time);
					GS_CORE_WARN("Transpiler: {}", transpiler_time);
				}
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

	LinearAllocator Application::m_AstAllocator = LinearAllocator(1024 * 1024 * 10);
	LinearAllocator Application::m_IRAllocator = LinearAllocator(1024 * 1024 * 10);
}