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

		llvm::Module* llvm_module;
		llvm::IRBuilder<>* llvm_builder;
		llvm::LLVMContext* llvm_ctx;

		Array<llvm::Function*> proc_to_llvm;

		std::unordered_map<GS_Type*, llvm::Type*> type_to_llvm;
		llvm::Type* llvm_ptr = nullptr;

		llvm::Type* llvm_i8 = nullptr;
		llvm::Type* llvm_i16 = nullptr;
		llvm::Type* llvm_i32 = nullptr;
		llvm::Type* llvm_i64 = nullptr;
		llvm::Type* llvm_float = nullptr;
		llvm::Type* llvm_double = nullptr;
		llvm::Type* llvm_void = nullptr;
		llvm::Type* llvm_Array = nullptr;
	};

	LLVM_Converter LLVM_Converter_Make(LLVM_Converter_Spec spec, Il_Program* program);
	bool LLVMC_Run(LLVM_Converter& lc);
}
