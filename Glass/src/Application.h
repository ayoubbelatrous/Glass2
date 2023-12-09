#pragma once

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
		bool DumpIR = false;
		bool NoLink = false;

		bool Run = false;

		bool OutputDll = false;

		std::vector<std::string> CIncludes;
		std::vector<std::string> CLibs;
	};

	enum class ExitCode : int
	{
		Normal = 0,
		InvalidCommandLineInput = -2,
		LexerError = -3,
		ParserError = -4,
		CompilerError = -5,
	};

	struct LinearAllocator
	{
		LinearAllocator() {
			m_Data = nullptr;
		}

		LinearAllocator(u64 pre_allocate) {
			m_Data = (uint8_t*)malloc(pre_allocate);
			memset(m_Data, 0, m_Capacity);
			m_Capacity = pre_allocate;
		}

		~LinearAllocator() {
			free(m_Data);
		}

		template<typename T>
		T* Allocate(const T& d) {

			if ((m_Size + sizeof(T)) > m_Capacity) {
				GS_CORE_ASSERT(0, "Out Of Memory");
			}

			void* dst = (void*)(m_Data + m_Size);

			new (dst) T(d);

			T* result = (T*)(m_Data + m_Size);

			m_Size += sizeof(T);

			return result;
		}

	private:
		u64 m_Capacity = 0;
		u64 m_Size = 0;
		uint8_t* m_Data = nullptr;
	};

	class Application
	{
	public:
		Application(const CommandLineArgs& CmdLineArgs);

		void OnStart();
		void OnShutdown();

		static void Init();

		const CompilerFile& GetCompilerFile(u64 id) const;
		const std::map<u64, CompilerFile>& GetCompilerFiles() const;

		static void Abort(ExitCode code = ExitCode::Normal, const std::string& message = "");
		static void FatalAbort(ExitCode code = ExitCode::InvalidCommandLineInput, const std::string& message = "");

		static ApplicationOptions ParseOptions(const CommandLineArgs& CmdLineArgs);

		template<typename T>
		static T* AllocateAstNode(const T& d) {
			return m_AstAllocator.Allocate<T>(d);
		}

		template<typename T>
		static T* AllocateIRNode(const T& d) {
			return m_IRAllocator.Allocate<T>(d);
		}

		template<typename T>
		static T* AllocateTypeNode(const T& d) {
			return m_TypeAllocator.Allocate<T>(d);
		}

		template<typename T>
		static T* AllocateAsmNode(const T& d) {
			return m_ASMAllocator.Allocate<T>(d);
		}

		static bool UseAlloca;

	private:

		static std::vector<CompilerFile> GenerateCompilationFiles(const std::vector<fs_path>& files);

		std::chrono::steady_clock::time_point m_LexerStart;
		std::chrono::steady_clock::time_point m_ParserStart;
		std::chrono::steady_clock::time_point m_CompilerStart;
		std::chrono::steady_clock::time_point m_TranspilerStart;

		std::chrono::steady_clock::time_point m_LexerEnd;
		std::chrono::steady_clock::time_point m_ParserEnd;
		std::chrono::steady_clock::time_point m_CompilerEnd;
		std::chrono::steady_clock::time_point m_TranspilerEnd;

		CommandLineArgs m_Arguments;
		ApplicationOptions m_Options;

		static LinearAllocator m_AstAllocator;
		static LinearAllocator m_IRAllocator;
		static LinearAllocator m_TypeAllocator;
		static LinearAllocator m_ASMAllocator;

		std::map<u64, CompilerFile> m_Sources;
	};

#define IR(x) Application::AllocateIRNode(x)
#define AST(x) Application::AllocateAstNode(x)
#define TYPE(x) Application::AllocateTypeNode(x)
#define ASMA(x) Application::AllocateAsmNode(x)
}