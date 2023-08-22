workspace "Glass"
    architecture "x64"
    configurations { "Debug", "Release" }
    flags
    {
        "MultiProcessorCompile"
    }

IncludeDir = {}
IncludeDir["SPD_LOG"] = "%{wks.location}/Glass/vendor/spdlog/include"
IncludeDir["LLVM"] = "%{wks.location}/Glass/vendor/llvm/include"

LibDir = {}
LibDir["LLVM"] = "%{wks.location}/Glass/vendor/llvm/lib"

include "Glass"