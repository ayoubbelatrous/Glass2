#pragma once

#include "FrontEnd/CompilerFile.h"
#include "FrontEnd/Ast.h"

namespace Glass
{
	inline const std::string FN_KWRD = "fn";

	class Parser
	{
	public:
		Parser(const CompilerFile& file);

		ModuleFile* CreateAST();

		Statement* ParseStatement();

		Statement* ParseDirective();

		Statement* ParseIf();
		Statement* ParseWhile();

		Statement* ParseTypeExpr();

		Statement* ParseScope();
		Statement* ParseArgumentList();
		Statement* ParseFunction();

		Statement* ParseVarDecl();

		Statement* ParseReturn();

		Statement* ParseStruct();

		Expression* ParseExpression();

		Statement* ParsePrimaryExpr();

		Expression* ParseAssignExpr();
		Expression* ParseAddExpr();
		Expression* ParseMulExpr();
		Expression* ParseCallExpr();
		Expression* ParseMemberExpr();
		Expression* ParseArrayAccessExpr();
		Expression* ParseTypeOfExpr();

		Operator GetOperator(const Token& token) {
			TokenType Type = token.Type;
			switch (Type)
			{
			case TokenType::Add:
				return Operator::Add;
				break;
			case TokenType::Subtract:
				return Operator::Subtract;
				break;
			case TokenType::Multiply:
				return Operator::Multiply;
				break;
			case TokenType::Divide:
				return Operator::Divide;
				break;
			case TokenType::Assign:
				return Operator::Assign;
				break;
			default:
				return Operator::Invalid;
				break;
			}
		}

	private:

		const Token& At(i64 ahead = 0) {
			return m_File.GetToken(m_File.GetTokens()[m_Location + ahead]);
		}

		const Token& Consume() {
			u64 loc = m_Location;
			m_Location++;
			return m_File.GetToken(m_File.GetTokens()[loc]);
		}

		bool ExpectedToken(TokenType Type, i64 ahead = 0) {
			if (At(ahead).Type != Type)
				return true;
			return false;
		}

		bool IsEndOfFile() {
			return At().Type == TokenType::E_OF;
		}

		void Abort(const std::string& message) {
			GS_CORE_WARN("{} '{}'", message, At().Symbol);
			GS_CORE_ERROR("\t{}:{}:{}", m_File.GetPath(), At().Line + 1, At().Begin);
			Application::FatalAbort(ExitCode::ParserError);
		}

		u64 m_Location = 0;

		const CompilerFile& m_File;
	};
}