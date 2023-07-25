#include "pch.h"

#include "FrontEnd/Parser.h"
#include "Application.h"

namespace Glass
{
	Statement* Parser::ParseStatement()
	{
		TokenType Type = At().Type;

		switch (Type)
		{
		case TokenType::Pound:
			Consume();
			return ParseDirective();
			break;
		case TokenType::StringLiteral:
		case TokenType::NumericLiteral:
		case TokenType::OpenParen:
			return ParseExpression();
			break;
		case TokenType::Symbol:
			if (At().Symbol == FN_KWRD) {
				return ParseFunction();
			}
			else if (At().Symbol == "ret" || At().Symbol == "return")
			{
				return ParseReturn();
			}
			else if (At().Symbol == "struct")
			{
				auto struct_ = ParseStruct();

				if (ExpectedToken(TokenType::SemiColon)) {
					Abort("Expected a ';' after struct declaration, Instead Got: ");
				}

				Consume();

				return struct_;
			}
			else if (At().Symbol == "if")
			{
				return ParseIf();
			}
			else if (At().Symbol == "while")
			{
				return ParseWhile();
			}
			else {

				bool var_decl = At().Type == TokenType::Symbol && At(1).Type == TokenType::Symbol;
				var_decl |= At().Type == TokenType::Symbol && At(1).Type == TokenType::Multiply && At(2).Type == TokenType::Symbol;
				var_decl |= At().Type == TokenType::Symbol && At(1).Type == TokenType::OpenBracket && At(2).Type == TokenType::NumericLiteral && At(3).Type == TokenType::CloseBracket && At(4).Type == TokenType::Symbol;

				if (var_decl) {
					return ParseVarDecl();
				}
				else {
					return ParseExpression();
				}
			}
			break;
		case TokenType::OpenCurly:
		{
			return ParseScope();
		}
		case TokenType::CloseCurly:
		{
			return nullptr;
		}
		break;
		case TokenType::BOL:
			Consume();
			return ParseStatement();
			break;
		}

		//if (ExpectedToken(TokenType::SemiColon)) {
		//	Abort("Expected ';' After Statement Instead Got")
		//}

		return ParseExpression();
	}

	Statement* Parser::ParseDirective()
	{
		if (At().Symbol == "foreign") {
			Consume();

			ForeignNode Node;

			Node.statement = ParseStatement();

			if (Node.statement == nullptr) {
				Abort("Expected A Statement After #foreign Directive");
			}

			return Application::AllocateAstNode(Node);
		}

		Abort(fmt::format("Un-recognized directive: {}", At().Symbol));

		return nullptr;
	}

	Statement* Parser::ParseIf()
	{
		Consume();

		IfNode Node;
		Node.Condition = ParseExpression();

		if (Node.Condition == nullptr) {
			Abort("Expected Condition After 'if' Instead Got");
		}

		Node.Scope = (ScopeNode*)ParseScope();

		return Application::AllocateAstNode(Node);
	}

	Statement* Parser::ParseWhile()
	{
		Consume();

		WhileNode Node;
		Node.Condition = ParseExpression();

		if (Node.Condition == nullptr) {
			Abort("Expected Condition After 'while' Instead Got");
		}

		Node.Scope = (ScopeNode*)ParseScope();

		return Application::AllocateAstNode(Node);
	}

	Statement* Parser::ParseTypeExpr()
	{
		if (ExpectedToken(TokenType::Symbol)) {
			return ParseStatement();
		}

		TypeExpression Node;
		Node.Symbol = Consume();

		if (At().Type == TokenType::OpenBracket) {
			Consume();
			if (ExpectedToken(TokenType::CloseBracket)) {
				Abort("Expected ']' after type Instead Got");
			}
			else {
				Consume();
				Node.Array = true;
			}
		}

		if (At().Type == TokenType::Multiply) {
			Consume();
			Node.Pointer = true;
		}

		return Application::AllocateAstNode(Node);
	}

	Statement* Parser::ParseVarDecl()
	{
		VariableNode Node;
		Node.Type = (TypeExpression*)ParseTypeExpr();

		if (ExpectedToken(TokenType::Symbol)) {
			Abort("Expected a variable name after type Instead Got");
		}

		Node.Symbol = Consume();

		if (At().Type == TokenType::Assign) {
			Consume();
			Node.Assignment = ParseExpression();
		}

		return Application::AllocateAstNode(Node);
	}

	Statement* Parser::ParseReturn()
	{
		ReturnNode Node;

		Node.RetSymbol = Consume();

		Node.Expr = ParseExpression();

		return Application::AllocateAstNode(Node);
	}

	Statement* Parser::ParseStruct()
	{
		Consume();

		if (ExpectedToken(TokenType::Symbol)) {
			Abort("Expected a struct name after 'struct' keyword, Instead Got: ");
		}

		StructNode Node;
		Node.Name = Consume();

		if (ExpectedToken(TokenType::OpenCurly)) {
			Abort("Expected a '{' after struct name, Instead Got: ");
		}

		Consume();

		while (At().Type != TokenType::CloseCurly) {
			Node.m_Members.push_back((VariableNode*)ParseVarDecl());
			if (ExpectedToken(TokenType::SemiColon)) {
				Abort("Expected a ';' after struct member declaration, Instead Got: ");
			}
			Consume();
		}

		Consume();

		return Application::AllocateIRNode(Node);
	}

	Statement* Parser::ParseScope()
	{
		if (ExpectedToken(TokenType::OpenCurly)) {
			return ParseStatement();
		}

		ScopeNode Node;
		Node.OpenCurly = Consume();

		while (At().Type != TokenType::CloseCurly) {
			auto stmt = ParseStatement();
			if (stmt != nullptr)
				Node.PushStatement(stmt);
		}

		if (ExpectedToken(TokenType::CloseCurly)) {
			Abort("Expected '{' Instead Got ");
		}

		Node.CloseCurly = Consume();

		return Application::AllocateAstNode(Node);
	}

	Statement* Parser::ParseArgumentList()
	{
		ArgumentList Node;

		Node.OpenParen = Consume();

		while (At().Type != TokenType::CloseParen) {

			auto arg = ParseVarDecl();

			Node.PushArgument(arg);

			if (At().Type == TokenType::Comma) {
				Consume();
				if (ExpectedToken(TokenType::Symbol)) {
					Abort("Expected symbol after ',' Instead Got");
				}
			}
		}

		Node.CloseParen = Consume();

		return Application::AllocateAstNode(Node);
	}

	Statement* Parser::ParseFunction()
	{
		FunctionNode Node;
		Node.DefinitionTk = Consume();

		if (ExpectedToken(TokenType::Symbol)) {
			Abort("Expected A Valid Name For Function Instead Got");
		}

		Node.Symbol = Consume();

		if (At().Type == TokenType::Bang) {
			Consume();
			Node.Variadic = true;
		}

		if (ExpectedToken(TokenType::OpenParen)) {
			Abort("Expected A '(' Instead Got ");
		}

		Node.SetArgList((ArgumentList*)ParseArgumentList());

		if (At().Type == TokenType::Colon) {
			Consume();
			if (ExpectedToken(TokenType::Symbol)) {
				Abort("Expected a type after ':' in function definition");
			}
			else {
				Node.ReturnType = (TypeExpression*)ParseTypeExpr();
			}
		}

		if (ExpectedToken(TokenType::OpenCurly)) {
			Abort("Expected A '{' During Function Parsing Instead Got");
		}

		Node.SetScope((ScopeNode*)ParseScope());

		return Application::AllocateAstNode(Node);
	}

	Expression* Parser::ParseExpression()
	{
		return ParseAssignExpr();
	}

	Expression* Parser::ParseAssignExpr()
	{
		Expression* left = ParseAddExpr();

		auto is_assignment_op = [](Operator op) -> bool {
			if (op == Operator::Invalid)
				return false;
			if (op == Operator::Assign)
				return true;

			return false;
		};

		while (is_assignment_op(GetOperator(At()))) {
			Token Op = Consume();
			auto right = ParseAddExpr();

			BinaryExpression binExpr;

			binExpr.Left = left;
			binExpr.Right = right;

			binExpr.OPerator = GetOperator(Op);

			binExpr.OperatorToken = Op;

			left = Application::AllocateAstNode(binExpr);
		}

		return left;
	}

	Expression* Parser::ParseAddExpr()
	{
		Expression* left = ParseMulExpr();

		auto is_additive_op = [](Operator op) -> bool {
			if (op == Operator::Invalid)
				return false;
			if (op == Operator::Add || op == Operator::Subtract)
				return true;

			return false;
		};

		while (is_additive_op(GetOperator(At()))) {
			Token Op = Consume();
			auto right = ParseMulExpr();

			BinaryExpression binExpr;

			binExpr.Left = left;
			binExpr.Right = right;

			binExpr.OPerator = GetOperator(Op);

			binExpr.OperatorToken = Op;

			left = Application::AllocateAstNode(binExpr);
		}

		return left;
	}

	Expression* Parser::ParseMulExpr()
	{
		Expression* left = ParseArrayAccessExpr();

		auto is_multiplicative_op = [](Operator op) -> bool {
			if (op == Operator::Invalid)
				return false;
			if (op == Operator::Multiply || op == Operator::Divide)
				return true;

			return false;
		};

		while (is_multiplicative_op(GetOperator(At()))) {

			Token Op = Consume();
			auto right = ParseArrayAccessExpr();

			BinaryExpression binExpr;

			binExpr.Left = left;
			binExpr.Right = right;

			binExpr.OPerator = GetOperator(Op);

			binExpr.OperatorToken = Op;

			left = Application::AllocateAstNode(binExpr);
		}

		return left;
	}

	Expression* Parser::ParseArrayAccessExpr()
	{
		Expression* object = ParseMemberExpr();

		if (At().Type == TokenType::OpenBracket) {

			Token bracket = Consume();

			Expression* index = ParseMemberExpr();

			Token close_bracket = Consume();

			ArrayAccess Node;

			Node.Object = object;
			Node.Index = index;

			object = Application::AllocateAstNode(Node);
		}

		return object;
	}

	Expression* Parser::ParseMemberExpr()
	{
		Expression* left = ParseCallExpr();

		while (At().Type == TokenType::Period) {

			Token period = Consume();

			Expression* right = ParseCallExpr();

			MemberAccess Node;

			Node.Object = left;
			Node.Member = right;

			left = Application::AllocateAstNode(Node);
		}

		return left;
	}

	Expression* Parser::ParseCallExpr()
	{
		Expression* name = (Expression*)ParsePrimaryExpr();
		if (name) {


			auto is_func_call = [this, name]() -> bool {
				if (name->GetType() == NodeType::Identifier && At().Type == TokenType::OpenParen) {
					return true;
				}
				else {
					return false;
				}
			};

			if (is_func_call()) {
				Consume();

				std::vector<Expression*> arguments;

				while (At().Type != TokenType::CloseParen) {
					arguments.push_back(ParseExpression());
					if (At().Type == TokenType::Comma) {
						Consume();
					}
				}

				Consume();

				if (name->GetType() == NodeType::Identifier) {

					FunctionCall* func_call = Application::AllocateAstNode(FunctionCall());
					func_call->Arguments = arguments;
					func_call->Function = ((Identifier*)name)->Symbol;

					name = func_call;
				}
			}
		}

		return name;
	}

	Statement* Parser::ParsePrimaryExpr()
	{
		TokenType Type = At().Type;
		switch (Type)
		{
		case TokenType::SemiColon:
		{
			Consume();
			return ParseStatement();
		}
		break;
		case TokenType::OpenParen:
		{
			Consume();
			auto expr = ParseExpression();

			{//ERRORS
				if (expr == nullptr) {
					Abort("Expected An Expression Inside Parentheses");
				}

				if (ExpectedToken(TokenType::CloseParen)) {
					Abort("Expected ')' Instead Got ");
				}
			}

			Consume();
			return expr;
		}
		break;
		case TokenType::Symbol:
		{
			Identifier identifier;
			identifier.Symbol = Consume();

			return Application::AllocateAstNode(identifier);
		}
		case TokenType::BOL:
		{
			Consume();
			return ParseExpression();
		}
		break;
		case TokenType::StringLiteral:
		{
			StringLiteral* lit = Application::AllocateAstNode(StringLiteral());
			lit->Symbol = Consume();
			return lit;
		}
		break;
		case TokenType::NumericLiteral:
		{
			NumericLiteral num_lit;
			num_lit.token = Consume();
			num_lit.Value = std::stoi(num_lit.token.Symbol);

			return Application::AllocateAstNode(num_lit);
		}
		break;
		}

		return nullptr;
	}

	ModuleFile* Parser::CreateAST()
	{
		ModuleFile* module_file = Application::AllocateAstNode(ModuleFile());

		while (!IsEndOfFile())
		{
			auto stmt = ParseStatement();

			if (stmt == nullptr) {
				return nullptr;
			}

			module_file->PushStatement(stmt);
		}

		return module_file;
	}

	Parser::Parser(const CompilerFile& file)
		:m_File(file)
	{
	}
}