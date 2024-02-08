project "Glass"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "on"

    targetdir "bin/%{cfg.buildcfg}"
    files { "src/**.h", "src/**.cpp", "src/**.asm", "src/**.nativs" }

    pchsource "src/pch.cpp"
    pchheader "pch.h"

    includedirs
    {
        "src",
        "vendor",
        "%{IncludeDir.SPD_LOG}",
        "%{IncludeDir.LLVM}",
    }

    libdirs
    {
        "%{LibDir.LLVM}",
    }

    links
	{
		--"LLVMSupport.lib",
        --"LLVMCore.lib",
        --"LLVMBinaryFormat.lib",
        --"LLVMRemarks.lib",
        --"LLVMBitstreamReader.lib",
        --"LLVMMC.lib",
        --"LLVMTarget.lib",
        --"LLVMCodeGen.lib",
        --"LLVMX86Info.lib",
        --"LLVMX86Desc.lib",
        --"LLVMX86CodeGen.lib",
        --"LLVMAsmPrinter.lib",

        "LLVMAggressiveInstCombine.lib",
        "LLVMAMDGPUAsmParser.lib",
        "LLVMAMDGPUCodeGen.lib",
        "LLVMAMDGPUDesc.lib",
        "LLVMAMDGPUDisassembler.lib",
        "LLVMAMDGPUInfo.lib",
        "LLVMAMDGPUUtils.lib",
        "LLVMAnalysis.lib",
        "LLVMAsmParser.lib",
        "LLVMAsmPrinter.lib",
        "LLVMBinaryFormat.lib",
        "LLVMBitReader.lib",
        "LLVMBitstreamReader.lib",
        "LLVMBitWriter.lib",
        "LLVMCFGuard.lib",
        "LLVMCFIVerify.lib",
        "LLVMCodeGen.lib",
        "LLVMCore.lib",
        "LLVMCoroutines.lib",
        "LLVMCoverage.lib",
        "LLVMDebugInfoCodeView.lib",
        "LLVMDebugInfoDWARF.lib",
        "LLVMDebugInfoGSYM.lib",
        "LLVMDebugInfoMSF.lib",
        "LLVMDebugInfoPDB.lib",
        "LLVMDemangle.lib",
        "LLVMDlltoolDriver.lib",
        "LLVMDWARFLinker.lib",
        "LLVMDWP.lib",
        "LLVMExecutionEngine.lib",
        "LLVMExegesis.lib",
        "LLVMExegesisX86.lib",
        "LLVMExtensions.lib",
        "LLVMFileCheck.lib",
        "LLVMFrontendOpenACC.lib",
        "LLVMFrontendOpenMP.lib",
        "LLVMFuzzMutate.lib",
        "LLVMGlobalISel.lib",
        "LLVMInstCombine.lib",
        "LLVMInstrumentation.lib",
        "LLVMInterfaceStub.lib",
        "LLVMInterpreter.lib",
        "LLVMipo.lib",
        "LLVMIRReader.lib",
        "LLVMJITLink.lib",
        "LLVMLibDriver.lib",
        "LLVMLineEditor.lib",
        "LLVMLinker.lib",
        "LLVMLTO.lib",
        "LLVMMC.lib",
        "LLVMMCA.lib",
        "LLVMMCDisassembler.lib",
        "LLVMMCJIT.lib",
        "LLVMMCParser.lib",
        "LLVMMIRParser.lib",
        "LLVMNVPTXCodeGen.lib",
        "LLVMNVPTXDesc.lib",
        "LLVMNVPTXInfo.lib",
        "LLVMObjCARCOpts.lib",
        "LLVMObject.lib",
        "LLVMObjectYAML.lib",
        "LLVMOption.lib",
        "LLVMOrcJIT.lib",
        "LLVMOrcShared.lib",
        "LLVMOrcTargetProcess.lib",
        "LLVMPasses.lib",
        "LLVMProfileData.lib",
        "LLVMRemarks.lib",
        "LLVMRuntimeDyld.lib",
        "LLVMScalarOpts.lib",
        "LLVMSelectionDAG.lib",
        "LLVMSupport.lib",
        "LLVMSymbolize.lib",
        "LLVMTableGen.lib",
        "LLVMTableGenGlobalISel.lib",
        "LLVMTarget.lib",
        "LLVMTextAPI.lib",
        "LLVMTransformUtils.lib",
        "LLVMVectorize.lib",
        "LLVMWindowsManifest.lib",
        "LLVMX86AsmParser.lib",
        "LLVMX86CodeGen.lib",
        "LLVMX86Desc.lib",
        "LLVMX86Disassembler.lib",
        "LLVMX86Info.lib",
        "LLVMXRay.lib",
        "LTO.lib",
        "Remarks.lib",
    }

    defines
	{
		"_CRT_SECURE_NO_WARNINGS",
        "_CRT_NONSTDC_NO_WARNINGS",
    }

    filter "files:src/microsoft_craziness.cpp"
	    flags { "NoPCH" }

    filter "system:windows"
	    systemversion "latest"
        defines "GS_WINDOWS"

    filter "configurations:Debug"
        defines { "DEBUG","GS_DEBUG" }

    symbols "On"
    filter "configurations:Release"
        defines { "NDEBUG","GS_RELEASE" }
        optimize "Speed"