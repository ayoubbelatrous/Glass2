#pragma once

#include "BackEnd/IR.h"
#include "BackEnd/Metadata.h"

namespace Glass
{
	struct X86_BackEnd_Data
	{
		std::unordered_map<u64, TypeStorage*> IR_RegisterTypes;
	};

	class X86_BackEnd
	{
	public:
		X86_BackEnd(IRTranslationUnit* translation_unit, MetaData* metadata);

		void Init();

		void Assemble();
		void Assemble(IRInstruction* inst);

		TypeStorage* GetIRNodeType(IRInstruction* ir);

		X86_BackEnd_Data m_Data;
		IRTranslationUnit* m_TranslationUnit = nullptr;
		MetaData* m_Metadata = nullptr;
	};
}