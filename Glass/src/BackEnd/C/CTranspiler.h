#pragma once

#include "BackEnd/IR.h"
#include "BackEnd/Compiler.h"

namespace Glass
{
	class CTranspiler
	{
	public:

		CTranspiler(IRTranslationUnit* program, const std::vector<std::string>& includes, const MetaData* metadata);

		std::string Codegen();

		std::string IRCodeGen(IRInstruction* inst);

		std::string TypeOfCodeGen(IRTypeOf* type_of);

		std::string FunctionCodeGen(IRFunction* IRF);
		std::string SSACodeGen(IRRegister* SSA);

		std::string StructCodeGen(IRStruct* ir_struct);

		std::string SSAValueCodeGen(IRRegisterValue* ssaVal);
		std::string OpCodeGen(IRInstruction* op);

		std::string CallCodeGen(IRFunctionCall* call);

		std::string GetType(u64 ID);

		u64 PushLabel() {
			m_LabelCounter++;
			return m_LabelCounter;
		}

		void PushLabelCode(const std::string& code) {
			m_LabelCode += code;
		}

		void ClearLabelCode() {
			m_LabelCode.clear();
			m_LabelCounter = 0;
		}

		void PushSSAHeader(const std::string& code) {
			m_SSAHeader += code;
		}

		void ClearSSAHeader() {
			m_SSAHeader.clear();
		}

	private:

		u64 m_LabelCounter = 0;
		std::string m_LabelCode;
		std::string m_SSAHeader;

		std::unordered_map <u64, u64> m_TypeInfoTable;

		const MetaData* m_Metadata = nullptr;

		std::unordered_map<u64, std::string> m_TypeMap;
		IRTranslationUnit* m_Program = nullptr;

		std::vector<std::string> m_Includes;
	};
}