#pragma once

#include "Base/String.h"
#include "Il.h"

namespace Glass
{
	struct LLVM_Converter_Spec
	{
		String output_path;
	};

	struct LLVM_Converter
	{
		Il_Program* prog;
		LLVM_Converter_Spec spec;
	};

	LLVM_Converter LLVM_Converter_Make(LLVM_Converter_Spec spec, Il_Program* program);
	bool LLVMC_Run(LLVM_Converter& lc);
}
