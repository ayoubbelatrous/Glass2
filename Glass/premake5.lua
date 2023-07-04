project "Glass"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"

    targetdir "bin/%{cfg.buildcfg}"
    files { "src/**.h", "src/**.cpp" }

    pchsource "src/pch.cpp"
    pchheader "pch.h"

    includedirs
    {
        "src",
        "vendor",
        "%{IncludeDir.SPD_LOG}",
    }

    defines
	{
		"_CRT_SECURE_NO_WARNINGS",
        "_CRT_NONSTDC_NO_WARNINGS",
    }

    filter "system:windows"
	    systemversion "latest"
        defines "GS_WINDOWS"

    filter "configurations:Debug"
        defines { "DEBUG","GS_DEBUG" }

    symbols "On"
    filter "configurations:Release"
        defines { "NDEBUG","GS_RELEASE" }
        optimize "On"