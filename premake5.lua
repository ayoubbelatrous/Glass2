workspace "Glass"
    architecture "x64"
    configurations { "Debug", "Release" }
    flags
    {
        "MultiProcessorCompile"
    }

IncludeDir = {}

IncludeDir["SPD_LOG"] = "%{wks.location}/Glass/vendor/spdlog/include"

include "Glass"