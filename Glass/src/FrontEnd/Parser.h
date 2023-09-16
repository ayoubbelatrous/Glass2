#pragma once

#include "FrontEnd/CompilerFile.h"
#include "FrontEnd/Ast.h"

#define AutoCastName "xx"

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
		Statement* ParseFor();

		Statement* ParseTypeExpr();
		Statement* ParseFuncTypeExpr();

		Statement* ParseScope();
		Statement* ParseArgument();
		Statement* ParseArgumentList();
		Statement* ParseFunction();

		Statement* ParseVarDecl();
		Statement* ParseVarDeclInfer();

		Statement* ParseReturn();

		Statement* ParseStruct();

		Statement* ParseEnum();

		Expression* ParseExpression();

		Statement* ParsePrimaryExpr();

		Expression* ParseDeRefExpr();
		Expression* ParseAssignExpr();
		Expression* ParseAddExpr();
		Expression* ParseCompExpr();
		Expression* ParseBitLogiExpr();
		Expression* ParseLogiExpr();
		Expression* ParseMulExpr();
		Expression* ParseCallExpr();
		Expression* ParseMemberExpr();
		Expression* ParseNegateExpr();
		Expression* ParseArrayAccessExpr();
		Expression* ParseRangeExpr();
		Expression* ParseTypeOfExpr();
		Expression* ParseCastExpr();
		Expression* ParseAutoCastExpr();
		Expression* ParseSizeOfExpr();

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
			case TokenType::Bang:
				return Operator::Not;
				break;
			case TokenType::OpenAngular:
				return Operator::LesserThan;
				break;
			case TokenType::CloseAngular:
				return Operator::GreaterThan;
				break;
			case TokenType::Equal:
				return Operator::Equal;
				break;
			case TokenType::NotEqual:
				return Operator::NotEqual;
				break;
			case TokenType::GreaterEq:
				return Operator::GreaterThanEq;
				break;
			case TokenType::LesserEq:
				return Operator::LesserThanEq;
				break;
			case TokenType::Ampersand:
				return Operator::BitAnd;
				break;
			case TokenType::Pipe:
				return Operator::BitOr;
				break;
			case TokenType::Symbol:
			{
				if (token.Symbol == "and") {
					return Operator::And;
				}
				else if (token.Symbol == "or") {
					return Operator::Or;
				}
				else {
					return Operator::Invalid;
				}
			}
			break;
			default:
				return Operator::Invalid;
				break;
			}
		}

	private:

		const Token& At(i64 ahead = 0) {
			return m_File.GetTokens()[m_Location + ahead];
		}

		const Token& Consume() {
			u64 loc = m_Location;
			m_Location++;
			return m_File.GetTokens()[loc];
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