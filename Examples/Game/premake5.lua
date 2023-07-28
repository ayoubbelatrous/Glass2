workspace "GameSln"
    architecture "x64"
    configurations { "Debug" }
    flags
    {
        -- "MultiProcessorCompile"
    }

project "Game"
    kind "ConsoleApp"
    language "C"

    targetdir "./"
    files { "generated.c" }

    defines
	{
		"_CRT_SECURE_NO_WARNINGS",
        "_CRT_NONSTDC_NO_WARNINGS",
    }

    libdirs
    {
        "glfw"
    }

    links
    {
        "opengl32.lib",
        "GLFW.lib"
    }