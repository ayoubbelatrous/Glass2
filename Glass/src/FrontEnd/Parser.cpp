#include "pch.h"

#include "FrontEnd/Parser.h"
#include "StrUtils.h"
#include "Application.h"

namespace Glass
{
	Statement* Parser::ParseStatement()
	{
		TokenType Type = At().Type;

		auto expected_semicolon = [this]() {
			if (ExpectedToken(TokenType::SemiColon)) {
				Abort("Expected ';' After Statement Instead Got");
			}
			Consume();
		};

		switch (Type)
		{
		case TokenType::Pound: {
			Consume();
			auto directive = ParseDirective();
			//expected_semicolon();
			return directive;
		}
		case TokenType::Symbol:
			if (At().Symbol == FN_KWRD) {
				return ParseFunction();
			}
			else if (At().Symbol == "return")
			{
				auto ret = ParseReturn();
				expected_semicolon();
				return ret;
			}
			else if (At().Symbol == "struct")
			{
				auto struct_ = ParseStruct();
				return struct_;
			}
			else if (At().Symbol == "if")
			{
				return ParseIf();
			}
			else if (At().Symbol == "enum")
			{
				auto enm = ParseEnum();
				return enm;
			}
			else if (At().Symbol == "while")
			{
				return ParseWhile();
			}
			else if (At().Symbol == "for")
			{
				return ParseFor();
			}
			else if (At().Symbol == "break")
			{
				BreakNode Node;

				Node.BR = Consume();

				expected_semicolon();

				return Application::AllocateAstNode(Node);
			}
			else {

				bool var_decl_infer = At().Type == TokenType::Symbol && At(1).Type == TokenType::Colon && At(2).Type == TokenType::Assign;
				var_decl_infer |= At().Type == TokenType::Symbol && At(1).Type == TokenType::Colon && At(2).Type == TokenType::Colon;

				Top_Level_Expression = false;

				if (var_decl_infer) {
					auto var = ParseVarDeclInfer();
					expected_semicolon();
					return var;
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
		case TokenType::SemiColon:
		{
			Consume();
			return ParseStatement();
		}
		break;
		case TokenType::BOL:
			Consume();
			return ParseStatement();
			break;
		case TokenType::E_OF:
			return nullptr;
			break;
		}

		Top_Level_Expression = true;
		auto expr = ParseExpression();

		if (expr) {
			if (expr->GetType() == NodeType::Identifier ||
				expr->GetType() == NodeType::TE_TypeName ||
				expr->GetType() == NodeType::TE_Pointer ||
				expr->GetType() == NodeType::TE_Array ||
				expr->GetType() == NodeType::ArrayAccess ||
				expr->GetType() == NodeType::Call ||
				expr->GetType() == NodeType::TE_Func) {
				if (At().Type == TokenType::Symbol) {
					return ParseVarDecl(expr);
				}
			}
		}

		expected_semicolon();

		return expr;
	}

	Statement* Parser::ParseDirective()
	{
		if (At().Symbol == "foreign") {
			Consume();

			Expression* library_name = ParseExpression();

			if (!library_name) {
				Abort("Expected library name after #foreign directive");
			}

			if (library_name->GetType() != NodeType::Identifier) {
				Abort("Expected library name to be a name after #foreign directive");
			}

			ForeignNode Node;

			//Node.statement = ParseStatement();
			Node.library_name = (Identifier*)library_name;

			// 			if (Node.statement == nullptr) {
			// 				Abort("Expected a Statement after library name");
			// 			}

			return Application::AllocateAstNode(Node);
		}

		if (At().Symbol == "operator") {
			Consume();

			OperatorNode Node;

			Operator op = GetOperator(Consume());

			if (op == Operator::Invalid) {
				Abort("Expected an Operator #operator directive");
			}

			Node.statement = ParseStatement();
			Node.OPerator = op;

			if (Node.statement == nullptr) {
				Abort("Expected A Function Name or Function definition After #operator Directive");
			}

			return Application::AllocateAstNode(Node);
		}

		if (At().Symbol == "load") {

			Consume();

			LoadNode Node;
			Node.FileName = (StringLiteral*)ParseExpression();

			if (Node.FileName == nullptr) {
				Abort("Expected A File Name after #load directive");
			}

			if (Node.FileName->GetType() != NodeType::StringLiteral) {
				Abort("Expected a string after #load directive");
			}

			return Application::AllocateAstNode(Node);
		}

		if (At().Symbol == "library") {

			Consume();

			LibraryNode Node;

			if (At().Type != TokenType::Symbol) {
				Abort("Expected a library name, Instead Got: ");
			}

			Node.Name = Consume();

			Node.FileName = (StringLiteral*)ParseStatement();

			if (Node.FileName == nullptr) {
				Abort("Expected A File Name after #library directive, Instead Got: ");
			}

			if (Node.FileName->GetType() != NodeType::StringLiteral) {
				Abort("Expected a string after #library directive, Instead Got: ");
			}

			return Application::AllocateAstNode(Node);
		}

		if (At().Symbol == "add_library") {

			Consume();

			AddLibraryNode Node;

			Node.FileName = (StringLiteral*)ParseStatement();

			if (Node.FileName == nullptr) {
				Abort("Expected a file name after #add_library directive, Instead Got: ");
			}

			if (Node.FileName->GetType() != NodeType::StringLiteral) {
				Abort("Expected a string after #add_library directive, Instead Got: ");
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

		Node.Scope = (ScopeNode*)ParseStatement();

		if (!Node.Scope) {
			Abort("Expected Expression Or Scope!, Before: ");
		}

		// 		if (Node.Scope->GetType() != NodeType::Scope) {
		// 			Abort("Expression if Statements not supported yet!, At: ");
		// 		}

		Node.Else = ParseElse();

		return Application::AllocateAstNode(Node);
	}

	ElseNode* Parser::ParseElse()
	{
		if (At().Symbol == "else") {

			Consume();
			ElseNode else_node;
			else_node.statement = ParseStatement();

			if (!else_node.statement) {
				Abort("Expected a statement after 'else' instead got:");
			}

			return AST(else_node);
		}
		else {
			return nullptr;
		}
	}

	Statement* Parser::ParseWhile()
	{
		Consume();

		WhileNode Node;
		Node.Condition = ParseExpression();

		if (Node.Condition == nullptr) {
			Abort("Expected condition after 'while' instead got");
		}

		Node.Scope = (ScopeNode*)ParseStatement();

		if (!Node.Scope) {
			Abort("Expected statement after while condition Instead Got:");
		}

		return Application::AllocateAstNode(Node);
	}

	Statement* Parser::ParseFor()
	{
		Consume();

		ForNode Node;

		Node.Condition = ParseExpression();

		if (Node.Condition == nullptr) {
			Abort("Expected something After 'for', Instead Got: ");
		}

		if (At(0).Type == TokenType::Colon) {
			Consume();

			if (Node.Condition->GetType() != NodeType::Identifier) {
				Abort("expected an identifier as the named iterator, Instead Got ");
			}

			Node.Named_Iterator = Node.Condition;
			Node.Condition = ParseExpression();
		}

		Node.Scope = (ScopeNode*)ParseScope();

		if (!Node.Scope) {
			Abort("for Expected Statement, Instead Got: ");
		}

		return Application::AllocateAstNode(Node);
	}

	Statement* Parser::ParseFuncTypeExpr()
	{
		ASSERT(nullptr);
		return nullptr;
	}

	Statement* Parser::ParseVarDecl(Expression* type /*= nullptr*/)
	{
		VariableNode Node;
		if (!type) {
			ASSERT(nullptr);
		}
		else {
			Node.Type = type;
		}

		if (ExpectedToken(TokenType::Symbol)) {
			Abort("Expected a variable name after type Instead Got");
		}

		if (At().Symbol == "if" || At().Symbol == "for" || At().Symbol == "struct" || At().Symbol == "enum" || At().Symbol == "fn" || At().Symbol == "return") {
			Abort("Invalid variable name:");
		}

		Node.Symbol = Consume();

		if (At().Type == TokenType::Assign) {
			Consume();
			Top_Level_Expression = false;
			Node.Assignment = ParseExpression();

			if (!Node.Assignment) {
				Abort("Expected Variable to be assigned something instead got:");
			}
		}

		if (At().Type == TokenType::Colon) {
			Consume();

			if (ExpectedToken(TokenType::Colon)) {
				Abort("Expected a second ':',Instead Got: ");
			}

			Consume();

			Node.Constant = true;

			if (At().Type == TokenType::Pound) {
				Consume();
				Node.Assignment = (Expression*)ParseDirective();
			}
			else {
				Top_Level_Expression = false;
				Node.Assignment = ParseExpression();
			}

			if (!Node.Assignment) {
				Abort("Expected Constant to be assigned something instead got:");
			}
		}

		return Application::AllocateAstNode(Node);
	}

	Statement* Parser::ParseVarDeclInfer()
	{
		VariableNode Node;
		Node.Type = nullptr;

		Node.Symbol = Consume();

		bool constant_decl = false;

		if (At().Type == TokenType::Colon) {
			Consume();
		}

		if (At().Type == TokenType::Colon) {
			Consume();
			constant_decl = true;
		}
		else if (At().Type == TokenType::Assign) {
			Consume();
		}

		if (constant_decl && At().Type == TokenType::Pound) {
			Consume();
			Top_Level_Expression = false;
			Node.Assignment = (Expression*)ParseDirective();
		}
		else {
			Top_Level_Expression = false;
			Node.Assignment = ParseExpression();
		}

		if (!Node.Assignment) {
			if (constant_decl) {
				Abort("Expected Constant to be assigned something instead got:");
			}
			else {
				Abort("Expected Inferred Variable to be assigned something instead got:");
			}
		}

		Node.Constant = constant_decl;

		return Application::AllocateAstNode(Node);
	}

	Statement* Parser::ParseReturn()
	{
		ReturnNode Node;

		Node.RetSymbol = Consume();

		if (At().Type != TokenType::SemiColon) {
			Node.Expr = ParseExpression();
		}

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

		if (At().Type == TokenType::OpenParen) {
			Node.argument_list = ((ArgumentList*)ParseArgumentList());
		}

		if (ExpectedToken(TokenType::OpenCurly)) {
			Abort("Expected a '{', Instead Got: ");
		}

		Consume();

		while (At().Type != TokenType::CloseCurly) {

			Top_Level_Expression = true;
			auto member = ParseVarDecl(ParseExpression());

			Node.m_Members.push_back((VariableNode*)member);
			if (ExpectedToken(TokenType::SemiColon)) {
				Abort("Expected a ';' after struct member declaration, Instead Got: ");
			}
			Consume();
		}

		Consume();

		return Application::AllocateIRNode(Node);
	}

	Statement* Parser::ParseEnum()
	{
		Consume();

		EnumNode Node;

		if (At().Type == TokenType::Pound) {

			Consume();

			if (ExpectedToken(TokenType::Symbol)) {
				Abort("Expected a directive name after '#', Instead Got: ");
			}

			if (At().Symbol == "flags") {
				Consume();
				Node.Flags = true;
			}
			else {
				GS_CORE_WARN("Supported enum directives are: #flags");
				Abort("Invalid Enum Directive:");
			}
		}

		if (ExpectedToken(TokenType::Symbol)) {
			Abort("Expected a name after enum keyword, Instead Got: ");
		}

		Node.Name = Consume();

		if (ExpectedToken(TokenType::OpenCurly)) {
			Abort("Expected a '{' after enum name, Instead Got: ");
		}

		Consume();

		while (At().Type != TokenType::CloseCurly)
		{
			Expression* expression = (Expression*)ParseStatement();

			if (expression == nullptr) {
				break;
			}

			Node.Members.push_back((Identifier*)expression);

			if (At().Type == TokenType::CloseCurly)
				break;
		}

		if (ExpectedToken(TokenType::CloseCurly)) {
			Abort("Expected a '}', Instead Got: ");
		}

		Consume();

		return AST(Node);
	}

	Statement* Parser::ParseScope()
	{
		if (ExpectedToken(TokenType::OpenCurly)) {
			return nullptr;
		}

		ScopeNode Node;
		Node.OpenCurly = Consume();

		while (At().Type != TokenType::CloseCurly) {

			if (At().Type == TokenType::E_OF) {
				Abort("reached the end of file looking for '}'");
			}

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

	Statement* Parser::ParseArgument()
	{
		ArgumentNode Node;
		Top_Level_Expression = true;
		Node.Type = ParseExpression();

		if (At().Type == TokenType::Dollar) {
			Consume();
			Node.PolyMorphic = true;
		}

		if (At().Type == TokenType::Period) {
			for (size_t i = 0; i < 3; i++)
			{
				if (ExpectedToken(TokenType::Period)) {
					Abort("Expected '.', Instead Got");
				}

				Consume();
			}

			Node.Variadic = true;
		}

		if (ExpectedToken(TokenType::Symbol)) {
			Abort("Expected a argument name after type Instead Got");
		}

		if (At().Symbol == "if" || At().Symbol == "for" || At().Symbol == "struct" || At().Symbol == "enum" || At().Symbol == "fn" || At().Symbol == "return") {
			Abort("Invalid argument name:");
		}

		Node.Symbol = Consume();

		if (At().Type == TokenType::Assign) {
			Abort("Default arguments are not yet supported");
		}

		return Application::AllocateAstNode(Node);
	}

	Statement* Parser::ParseArgumentList()
	{
		ArgumentList Node;

		Node.OpenParen = Consume();

		while (At().Type != TokenType::CloseParen) {

			auto arg = ParseArgument();

			Node.PushArgument(arg);

			if (At().Type == TokenType::Comma) {
				Consume();
				if (At().Type == TokenType::CloseParen) {
					Abort("Expected argument after, Instead Got: ");
				}
			}
			else {
				if (At().Type != TokenType::CloseParen) {
					Abort("Expected ',' or ')' after argument, Instead Got: ");
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
			Node.CVariadic = true;
		}

		if (ExpectedToken(TokenType::OpenParen)) {
			Abort("Expected A '(' Instead Got ");
		}

		Node.SetArgList((ArgumentList*)ParseArgumentList());

		if (At().Type == TokenType::Colon) {
			Consume();

			Top_Level_Expression = true;
			Node.ReturnType = ParseExpression();

			if (!Node.ReturnType) {
				Abort("Expected a type after ':' in function definition, Instead Got: ");
			}
		}

		if (At().Type != TokenType::SemiColon) {

			if (At().Type == TokenType::Pound) {
				Consume();

				Node.foreign_directive = (ForeignNode*)ParseDirective();

				if (!Node.foreign_directive) {
					Abort("Expected a valid directive after '#'");
				}

				if (Node.foreign_directive->GetType() != NodeType::Foreign) {
					Abort("Expected only supported directive here is: #foreign");
				}
			}
			else {
				if (ExpectedToken(TokenType::OpenCurly)) {
					Abort("Expected A '{' During Function Parsing Instead Got");
				}

				Node.SetScope((ScopeNode*)ParseScope());
			}
		}

		return Application::AllocateAstNode(Node);
	}

	Expression* Parser::ParseExpression()
	{
		return ParseRangeExpr();
	}

	Expression* Parser::ParseRangeExpr()
	{
		Expression* begin = ParseAssignExpr();

		while (At().Type == TokenType::Period && At(1).Type == TokenType::Period) {

			Token period = Consume();
			period = Consume();

			Expression* end = ParseAssignExpr();

			RangeNode Node;

			Node.Begin = begin;
			Node.End = end;

			begin = Application::AllocateAstNode(Node);
		}

		return begin;
	}

	Expression* Parser::ParseAssignExpr()
	{
		Expression* left = ParseBitLogiExpr();

		auto is_assignment_op = [](Operator op) -> bool {
			if (op == Operator::Invalid)
				return false;

			if (op == Operator::Assign ||
				op == Operator::AddAssign || op == Operator::SubAssign
				|| op == Operator::MulAssign || op == Operator::DivAssign
				|| op == Operator::BitAndAssign || op == Operator::BitOrAssign)
				return true;

			return false;
		};

		while (is_assignment_op(GetOperator(At()))) {
			Token Op = Consume();

			Top_Level_Expression = false;
			auto right = ParseBitLogiExpr();

			BinaryExpression binExpr;

			binExpr.Left = left;
			binExpr.Right = right;

			if (!binExpr.Right) {
				Abort("Expected Expression!, Before:");
			}

			binExpr.OPerator = GetOperator(Op);

			binExpr.OperatorToken = Op;

			left = Application::AllocateAstNode(binExpr);
		}

		return left;
	}

	Expression* Parser::ParseBitLogiExpr()
	{
		Expression* left = ParseLogiExpr();

		auto is_logical_op = [](Operator op) -> bool {
			if (op == Operator::Invalid)
				return false;

			if (op == Operator::BitAnd || op == Operator::BitOr)
				return true;

			return false;
		};

		while (is_logical_op(GetOperator(At()))) {

			Token Op = Consume();
			auto right = ParseLogiExpr();

			BinaryExpression binExpr;

			binExpr.Left = left;
			binExpr.Right = right;

			if (!binExpr.Right) {
				Abort("Expected Expression!, Before: ");
			}

			binExpr.OPerator = GetOperator(Op);

			binExpr.OperatorToken = Op;

			left = Application::AllocateAstNode(binExpr);
		}

		return left;
	}


	Expression* Parser::ParseLogiExpr()
	{
		Expression* left = ParseCompExpr();

		auto is_logical_op = [](Operator op) -> bool {
			if (op == Operator::Invalid)
				return false;

			if (op == Operator::And || op == Operator::Or)
				return true;

			return false;
		};

		while (is_logical_op(GetOperator(At()))) {

			Token Op = Consume();
			auto right = ParseCompExpr();

			BinaryExpression binExpr;

			binExpr.Left = left;
			binExpr.Right = right;

			if (!binExpr.Right) {
				Abort("Expected Expression!, Before: ");
			}

			binExpr.OPerator = GetOperator(Op);

			binExpr.OperatorToken = Op;

			left = Application::AllocateAstNode(binExpr);
		}

		return left;
	}

	Expression* Parser::ParseCompExpr()
	{
		Expression* left = ParseAddExpr();

		auto is_comp_op = [](Operator op) -> bool {

			if (op == Operator::Invalid)
				return false;

			if (op == Operator::GreaterThan || op == Operator::LesserThan)
				return true;
			if (op == Operator::Equal || op == Operator::NotEqual)
				return true;
			if (op == Operator::GreaterThanEq || op == Operator::LesserThanEq)
				return true;

			return false;
		};

		while (is_comp_op(GetOperator(At()))) {
			Token Op = Consume();
			auto right = ParseAddExpr();

			BinaryExpression binExpr;

			binExpr.Left = left;
			binExpr.Right = right;

			if (!binExpr.Right) {
				Abort("Expected Expression!, Before: ");
			}

			binExpr.OPerator = GetOperator(Op);

			binExpr.OperatorToken = Op;

			left = AST(binExpr);
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

			if (!binExpr.Right) {
				Abort("Expected Expression!, Before: ");
			}

			binExpr.OPerator = GetOperator(Op);

			binExpr.OperatorToken = Op;

			left = Application::AllocateAstNode(binExpr);
		}

		return left;
	}

	Expression* Parser::ParseMulExpr()
	{
		Expression* left = ParseNegateExpr();

		auto is_multiplicative_op = [](Operator op) -> bool {
			if (op == Operator::Invalid)
				return false;
			if (op == Operator::Multiply || op == Operator::Divide || op == Operator::Modulo)
				return true;

			return false;
		};

		while (is_multiplicative_op(GetOperator(At()))) {

			Token Op = Consume();
			auto right = ParseNegateExpr();

			BinaryExpression binExpr;

			binExpr.Left = left;
			binExpr.Right = right;

			if (!binExpr.Right) {
				Abort("Expected Expression!, Before: ");
			}

			binExpr.OPerator = GetOperator(Op);

			binExpr.OperatorToken = Op;

			left = Application::AllocateAstNode(binExpr);
		}

		return left;
	}

	Expression* Parser::ParseNegateExpr()
	{
		Expression* what = nullptr;

		if (At().Type == TokenType::Subtract) {

			Consume();

			what = ParseRefExpr();

			NegateExpr Node;
			Node.What = what;

			if (!Node.What) {
				Abort("Expected Expression!, Before: ");
			}

			what = (Expression*)AST(Node);
		}
		else {
			what = ParseRefExpr();
		}

		return what;
	}

	Expression* Parser::ParseRefExpr()
	{
		Expression* right = nullptr;

		if (At().Type == TokenType::Ampersand) {

			Consume();

			right = ParsePointerExpr();

			RefNode Node;
			Node.What = right;

			if (!Node.What) {
				Abort("Expected Expression!, Before: ");
			}

			right = (Expression*)AST(Node);
		}
		else {
			right = ParsePointerExpr();
		}

		return right;
	}

	Expression* Parser::ParsePointerExpr()
	{
		Expression* right = nullptr;

		if (At().Type == TokenType::Multiply || At().Type == TokenType::OpenBracket || At().Type == TokenType::OpenParen) {

			if (At().Type == TokenType::Multiply) {

				Consume();

				right = ParseExpression();

				PointerExpr Node;
				Node.Indirection = 1;
				Node.Pointee = right;

				right = (Expression*)AST(Node);
			}
			else if (At().Type == TokenType::OpenBracket) {

				Consume();

				bool dynamic = false;

				if (At().Type == TokenType::Period) {
					Consume();

					if (At().Type == TokenType::Period) {
						Consume();
						dynamic = true;
					}
				}

				if (!dynamic) {
					right = ParseExpression();
				}

				if (ExpectedToken(TokenType::CloseBracket)) {
					Abort("expected closing bracket ']' while parsing array type expression!");
				}

				Consume();

				ArrayTypeExpr Node;
				Node.ElementType = ParseExpression();
				Node.Size = right;
				Node.Dynamic = dynamic;

				right = (Expression*)AST(Node);
			}
			else {

				int i = 1;
				while (At(i).Type != TokenType::E_OF) {
					if (At(i).Type == TokenType::CloseParen) {
						if (At(i + 1).Type != TokenType::Colon) {
							return ParseDeRefExpr();
						}
						else {
							break;
						}
					}
					i++;
				}

				Consume();

				FuncExpr Node;

				while (At().Type != TokenType::E_OF) {

					auto expr = ParseExpression();

					if (!expr && At().Type != TokenType::CloseParen) {
						Abort("expected parameter type while parsing function type parameter list!, instead got:");
					}

					if (!expr && At().Type == TokenType::CloseParen) {
						Consume();
						break;
					}

					Node.Arguments.push_back(expr);

					if (At().Type == TokenType::CloseParen) {
						Consume();
						break;
					}

					if (ExpectedToken(TokenType::Comma)) {
						Abort("expected comma ',' while parsing function type expression!, instead got:");
					}

					Consume();
				}

				if (ExpectedToken(TokenType::Colon)) {
					Abort("expected colon ':' while parsing function type expression!, instead got:");
				}

				Consume();

				auto return_type_expr = ParseExpression();

				if (!return_type_expr) {
					Abort("expected return type while parsing function type!, instead got:");
				}

				Node.ReturnType = return_type_expr;

				right = (Expression*)AST(Node);

				i++;
			}
		}
		else {
			right = ParseDeRefExpr();
		}

		return right;
	}

	Expression* Parser::ParseDeRefExpr()
	{
		Expression* right = nullptr;

		if (At().Type == TokenType::OpenAngular) {

			Consume();

			right = ParseNotExpr();

			if (!right) {
				m_Location--;
				return right;
			}

			DeRefNode Node;
			Node.What = right;

			if (!Node.What) {
				Abort("Expected Expression!, Before: ");
			}

			right = (Expression*)AST(Node);
		}

		if (!right) {
			right = ParseNotExpr();
		}

		return right;
	}

	Expression* Parser::ParseNotExpr()
	{
		Expression* right = nullptr;

		if (At().Type == TokenType::Bang) {

			Consume();

			right = ParseMemberExpr((Expression*)ParsePrimaryExpr());

			if (!right) {
				m_Location--;
				return right;
			}

			NotExpr Node;
			Node.What = right;

			if (!Node.What) {
				Abort("Expected Expression!, Before: ");
			}

			right = (Expression*)AST(Node);
		}

		if (!right) {
			right = (Expression*)ParsePrimaryExpr();
		}

		if (At().Type == TokenType::OpenParen || At().Type == TokenType::OpenBracket || At().Type == TokenType::Period) {
			return ParseMemberExpr(right);
		}

		return right;
	}

	Expression* Parser::ParseMemberExpr(Expression* object)
	{
		Expression* left = object;

		while (At().Type == TokenType::Period && At(1).Type != TokenType::Period) {

			Token period = Consume();

			Expression* right = (Expression*)ParsePrimaryExpr();

			if (!right) {
				Abort("Expected member after '.' , Before: ");
			}

			if (right->GetType() != NodeType::Identifier) {
				Abort("Expected identifier after '.' , Instead Got: ");
			}

			MemberAccess Node;

			Node.Object = left;
			Node.Member = right;

			left = Application::AllocateAstNode(Node);
		}

		if (At().Type == TokenType::OpenParen || At().Type == TokenType::OpenBracket) {
			return ParseArrayAccessExpr(left);
		}

		return left;
	}

	Expression* Parser::ParseArrayAccessExpr(Expression* accessee)
	{
		Expression* object = accessee;

		while (At().Type == TokenType::OpenBracket) {

			if (At(2).Type == TokenType::Period && At(1).Type == TokenType::Period) {
				return accessee;
			}

			Token bracket = Consume();

			Top_Level_Expression = false;
			Expression* index = ParseExpression();

			Token close_bracket = Consume();

			if (!index) {
				Abort("Expected an expression after '[', Instead Got: ");
			}

			if (close_bracket.Type != TokenType::CloseBracket) {
				Abort("Expected ']', Instead Got: ");
			}

			ArrayAccess Node;

			Node.Object = object;
			Node.Index = index;

			object = Application::AllocateAstNode(Node);
		}

		if (At().Type == TokenType::OpenParen) {
			return ParseCallExpr(object);
		}

		if (At().Type == TokenType::Period) {
			return ParseMemberExpr(object);
		}

		return object;
	}

	Expression* Parser::ParseCallExpr(Expression* callee)
	{
		while (At().Type == TokenType::OpenParen) {

			Consume();

			std::vector<Expression*> arguments;
			Top_Level_Expression = false;

			while (At().Type != TokenType::CloseParen) {

				auto expr = ParseExpression();

				if (expr) {
					arguments.push_back(expr);
				}
				else {
					Abort("Expected an argument in function call, Instead Got: ");
				}

				if (At().Type == TokenType::Comma) {
					Consume();
				}
			}

			if (ExpectedToken(TokenType::CloseParen)) {
				Abort("Expected ')' on function call, Instead Got: ");
			}

			Consume();

			FunctionCall* func_call = Application::AllocateAstNode(FunctionCall());
			func_call->Arguments = arguments;
			func_call->callee = callee;

			callee = func_call;
		}

		if (At().Type == TokenType::OpenBracket || At().Type == TokenType::Period) {
			return ParseMemberExpr(callee);
		}

		return callee;
	}

	Statement* Parser::ParsePrimaryExpr()
	{
		TokenType Type = At().Type;

		switch (Type)
		{
		case TokenType::SemiColon:
		{
			return nullptr;
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
			if (At().Symbol == "if" || At().Symbol == "for" || At().Symbol == "struct" || At().Symbol == "enum" || At().Symbol == "fn" || At().Symbol == "return") {
				Abort("Invalid expression level name:");
			}

			if (At().Symbol == "cast") {
				return ParseCastExpr();
			}
			else if (At().Symbol == "sizeof") {
				return ParseSizeOfExpr();
			}
			else if (At().Symbol == AutoCastName) {
				return ParseAutoCastExpr();
			}
			else {

				// 				auto third_type = At(2).Type;
				// 				auto third_correct =
				// 					third_type != TokenType::Symbol &&
				// 					third_type != TokenType::NumericLiteral &&
				// 					third_type != TokenType::OpenParen;
				// 
				// 				if (
				// 					(At(1).Type == TokenType::Multiply || (At(1).Type == TokenType::OpenBracket && At(2).Type == TokenType::Period)) &&
				// 					third_correct
				// 					) {
				// 
				// 					return ParseTypeExpr();
				// 				}

				Identifier identifier;
				identifier.Symbol = Consume();

				return AST(identifier);
			}
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

			if (FindStringIC(num_lit.token.Symbol, ".")) {
				num_lit.Val.Float = std::stod(num_lit.token.Symbol);
				num_lit.type = NumericLiteral::Type::Float;
			}
			else {
				num_lit.Val.Int = std::stoull(num_lit.token.Symbol);
				num_lit.type = NumericLiteral::Type::Int;
			}

			return Application::AllocateAstNode(num_lit);
		}
		break;
		case TokenType::HexLiteral:
		{
			NumericLiteral num_lit;
			num_lit.token = Consume();

			num_lit.Val.Int = std::stoull(num_lit.token.Symbol.data() + 2, nullptr, 16);
			num_lit.type = NumericLiteral::Type::Int;

			return Application::AllocateAstNode(num_lit);
		}
		break;
		case TokenType::OpenAngular:
		{
			return ParseDeRefExpr();
		}
		break;
		}

		return nullptr;
	}

	Expression* Parser::ParseTypeOfExpr()
	{
		TypeOfNode Node;

		if (ExpectedToken(TokenType::OpenParen)) {
			Abort("Expected '(' after 'typeof'");
		}

		Consume();

		Node.What = ParseExpression();

		if (!Node.What) {
			Abort("Expected something inside parenthesis '()' of 'typeof'");
		}

		if (ExpectedToken(TokenType::CloseParen)) {
			Abort("Expected ')' after 'typeof' contents");
		}

		Consume();

		return Application::AllocateAstNode(Node);
	}

	Expression* Parser::ParseCastExpr()
	{
		CastNode Node;

		if (ExpectedToken(TokenType::Symbol)) {
			return ParseExpression();
		}

		Consume();

		if (ExpectedToken(TokenType::OpenParen)) {
			Abort("Expected '(' on cast expression");
		}

		Consume();

		Node.Type = ParseExpression();

		if (ExpectedToken(TokenType::CloseParen)) {
			Abort("Expected '(' on cast expression");
		}

		Consume();

		Node.Expr = ParseExpression();

		return AST(Node);
	}

	Expression* Parser::ParseAutoCastExpr()
	{
		AutoCastNode auto_cast_node;

		Consume(); // AutoCastName

		auto_cast_node.Expr = ParseExpression();

		if (!auto_cast_node.Expr) {
			Abort("Expected something after auto cast, Instead got: ");
		}

		return AST(auto_cast_node);
	}

	Expression* Parser::ParseSizeOfExpr()
	{
		SizeOfNode Node;
		Consume();

		if (ExpectedToken(TokenType::OpenParen)) {
			Abort("Expected '(' after sizeof, Instead Got:");
		}

		Consume();

		Node.Expr = ParseExpression();

		if (Node.Expr == nullptr) {
			Abort("Expected something after 'sizeof(', Instead Got:");
		}

		if (ExpectedToken(TokenType::CloseParen)) {
			Abort("Expected ')' after sizeof(..., Instead Got:");
		}

		Consume();

		return AST(Node);
	}

	ModuleFile* Parser::CreateAST()
	{
		ModuleFile* module_file = Application::AllocateAstNode(ModuleFile());

		while (!IsEndOfFile())
		{
			auto stmt = ParseStatement();

			if (stmt == nullptr) {
				return module_file;
			}

			module_file->PushStatement(stmt);
		}

		return module_file;
	}

	Parser::Parser(const CompilerFile& file)
		:Tokens(file.GetTokens())
	{
		ASSERT_UNIMPL();
	}

	Parser::Parser(const fs_path& path, std::vector<Token>& tokens)
		: Path(path), Tokens(tokens)
	{
	}
}