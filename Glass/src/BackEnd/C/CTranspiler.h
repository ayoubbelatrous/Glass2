#pragma once

#include "BackEnd/IR.h"
#include "BackEnd/Compiler.h"

namespace Glass
{
	class CTranspiler
	{
	public:

		CTranspiler(IRTranslationUnit* program, const Compiler::MetaData* metadata);

		std::string Codegen();

		std::string IRCodeGen(IRInstruction* inst);

		std::string FunctionCodeGen(IRFunction* IRF);
		std::string SSACodeGen(IRSSA* SSA);

		std::string StructCodeGen(IRStruct* ir_struct);

		std::string SSAValueCodeGen(IRSSAValue* ssaVal);
		std::string OpCodeGen(IRInstruction* op);

		std::string CallCodeGen(IRFunctionCall* call);

		std::string GetType(u64 ID);

	private:

		const Compiler::MetaData* m_Metadata = nullptr;

		std::unordered_map<u64, std::string> m_TypeMap;

		IRTranslationUnit* m_Program = nullptr;
	};
}