#pragma once

#include "Base/Base.h"
// This ignores all warnings raised inside External headers
#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#pragma warning(pop)

namespace Glass {

	class Log
	{
	public:
		static void Init();

		static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};

}

// Core log macros
#define GS_CORE_TRACE(...)    ::Glass::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define GS_CORE_INFO(...)     ::Glass::Log::GetCoreLogger()->info(__VA_ARGS__)
#define GS_CORE_WARN(...)     ::Glass::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define GS_CORE_ERROR(...)    ::Glass::Log::GetCoreLogger()->error(__VA_ARGS__)
#define GS_CORE_CRITICAL(...) ::Glass::Log::GetCoreLogger()->critical(__VA_ARGS__)

#include "Base/String.h"

template<typename OStream>
inline OStream& operator<<(OStream& os, const Glass::String& str)
{
	return os << std::string(str.data, str.count);
}
