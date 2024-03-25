#pragma once

#define GS_INTERNAL_ASSERT_IMPL(type, check, msg, ...) { if(!(check)) { GS##type##ERROR(msg, __VA_ARGS__); GS_DEBUGBREAK(); } }
#define GS_INTERNAL_ASSERT_WITH_MSG(type, check, ...) GS_INTERNAL_ASSERT_IMPL(type, check, "Assertion failed: {0}", __VA_ARGS__)
#define GS_INTERNAL_ASSERT_NO_MSG(type, check) GS_INTERNAL_ASSERT_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", GS_STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)

#define GS_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
#define GS_INTERNAL_ASSERT_GET_MACRO(...) GS_EXPAND_MACRO( GS_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, GS_INTERNAL_ASSERT_WITH_MSG, GS_INTERNAL_ASSERT_NO_MSG) )

#define GS_ASSERT(...) GS_EXPAND_MACRO( GS_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, __VA_ARGS__) )
#define GS_CORE_ASSERT(...) GS_EXPAND_MACRO( GS_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_CORE_, __VA_ARGS__) )
#define ASSERT_UNIMPL() GS_CORE_ASSERT(nullptr,"un-implemented!")
#define ASSERT(...) GS_EXPAND_MACRO( GS_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_CORE_, __VA_ARGS__) )

