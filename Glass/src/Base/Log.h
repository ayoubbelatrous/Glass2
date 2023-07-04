#pragma once

#include "Base/Base.h"

// This ignores all warnings raised inside External headers
#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#pragma warning(pop)

namespace Glass {

	enum class ConsoleMessageType
	{
		Info,
		Warn,
		Error,
	};

	struct ConsoleMessage
	{
		std::string Message;
		ConsoleMessageType Type;
	};

	class Log
	{
	public:
		static void Init();

		static Ref<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		static Ref<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
	private:
		static Ref<spdlog::logger> s_CoreLogger;
		static Ref<spdlog::logger> s_ClientLogger;
	};

}

// Core log macros
#define GS_CORE_TRACE(...)    ::Glass::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define GS_CORE_INFO(...)     ::Glass::Log::GetCoreLogger()->info(__VA_ARGS__)
#define GS_CORE_WARN(...)     ::Glass::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define GS_CORE_ERROR(...)    ::Glass::Log::GetCoreLogger()->error(__VA_ARGS__)
#define GS_CORE_CRITICAL(...) ::Glass::Log::GetCoreLogger()->critical(__VA_ARGS__)