#include "pch.h"

#include "LLVM_Converter.h"

#include "LLVM_Converter.h"

namespace Glass
{
	LLVM_Converter LLVM_Converter_Make(LLVM_Converter_Spec spec, Il_Program* program)
	{
		LLVM_Converter converter;
		converter.prog = program;
		converter.spec = spec;
		return converter;
	}

	void LLVMC_Codegen(LLVM_Converter& lc) {

		for (size_t i = 0; i < lc.prog->procedures.count; i++)
		{

		}

	}

	bool LLVMC_Run(LLVM_Converter& lc)
	{
		return true;
	}
}
