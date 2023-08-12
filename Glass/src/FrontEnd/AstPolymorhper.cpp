#include "pch.h"

#include "FrontEnd/AstPolymorpher.h"

namespace Glass
{
	ASTPolyMorpher::ASTPolyMorpher(Statement* statement, std::unordered_map <std::string, std::string> replacements)
		:m_Statement(statement), m_Replacements(replacements)
	{}

	void ASTPolyMorpher::Poly()
	{
		PolyStatement(m_Statement);
	}


	void ASTPolyMorpher::ReplaceIfMatch(std::string& str)
	{
		auto it = m_Replacements.find(str);
		if (it != m_Replacements.end()) {
			str = it->second;
		}
	}

	void ASTPolyMorpher::PolyStatement(Statement* stmt)
	{
		NodeType type = stmt->GetType();

		switch (type)
		{
		case NodeType::Identifier:
		case NodeType::NumericLiteral:
		case NodeType::StringLiteral:
		case NodeType::BinaryExpression:
		case NodeType::TypeExpression:
		case NodeType::MemberAccess:
		case NodeType::ArrayAccess:
		case NodeType::Reference:
		case NodeType::DeReference:
		case NodeType::Call:
			PolyExpression((Expression*)stmt);
			break;
		case NodeType::TypeOf:
		{
			TypeOfNode* type_of = (TypeOfNode*)stmt;
			PolyExpression(type_of->What);
		}
		break;

		case NodeType::ArgumentList:
			PolyArgumentList((ArgumentList*)stmt);
			break;
		case NodeType::Function:
			PolyFunction((FunctionNode*)stmt);
			break;
		case NodeType::Scope:
			PolyScope((ScopeNode*)stmt);
			break;

		case NodeType::Variable:
			PolyVariable((VariableNode*)stmt);
			break;

		case NodeType::Return:
			PolyReturn((ReturnNode*)stmt);
			break;

		case NodeType::If:
			PolyIf((IfNode*)stmt);
			break;
		case NodeType::While:
			PolyWhile((WhileNode*)stmt);
			break;
		case NodeType::For:
			break;
		}
	}

	void ASTPolyMorpher::PolyExpression(Expression* expr)
	{
		NodeType type = expr->GetType();

		switch (type)
		{
		case NodeType::Identifier:
		{
			Identifier* identifier = (Identifier*)expr;
			ReplaceIfMatch(identifier->Symbol.Symbol);
		}
		break;
		case NodeType::BinaryExpression:
			PolyBinaryExpression((BinaryExpression*)expr);
			break;
		case NodeType::TypeExpression:
			PolyTypeExpr((TypeExpression*)expr);
			break;
		case NodeType::MemberAccess:
			PolyMemberAccess((MemberAccess*)expr);
			break;
		case NodeType::ArrayAccess:
			PolyArrayAccess((ArrayAccess*)expr);
			break;
		case NodeType::Reference:
			PolyRef((RefNode*)expr);
			break;
		case NodeType::DeReference:
			PolyDeRef((DeRefNode*)expr);
			break;
		case NodeType::Call:
			PolyCallExpr((FunctionCall*)expr);
			break;
		}
	}

	void ASTPolyMorpher::PolyFunction(FunctionNode* func)
	{
		PolyStatement(func->GetArgList());

		for (Statement* statement : func->GetScope()->GetStatements()) {
			PolyStatement(statement);
		}

		if (func->ReturnType)
			PolyStatement(func->ReturnType);
	}

	void ASTPolyMorpher::PolyScope(ScopeNode* scope)
	{
	}

	void ASTPolyMorpher::PolyArgumentList(ArgumentList* arg_list)
	{
		for (Statement* a : arg_list->GetArguments()) {
			VariableNode* arg = (VariableNode*)a;
			arg->Type->PolyMorphic = false;
			ReplaceIfMatch(arg->Type->Symbol.Symbol);
		}
	}

	void ASTPolyMorpher::PolyVariable(VariableNode* var)
	{
		PolyTypeExpr(var->Type);
		if (var->Assignment)
			PolyStatement(var->Assignment);
	}

	void ASTPolyMorpher::PolyReturn(ReturnNode* ret)
	{
	}

	void ASTPolyMorpher::PolyIf(IfNode* ifNode)
	{
	}

	void ASTPolyMorpher::PolyWhile(WhileNode* whil)
	{
	}

	void ASTPolyMorpher::PolyBinaryExpression(BinaryExpression* binExpr)
	{
	}

	void ASTPolyMorpher::PolyMemberAccess(MemberAccess* expr)
	{
	}

	void ASTPolyMorpher::PolyCallExpr(FunctionCall* expr)
	{
	}

	void ASTPolyMorpher::PolyTypeExpr(TypeExpression* expr)
	{
		ReplaceIfMatch(expr->Symbol.Symbol);
	}

	void ASTPolyMorpher::PolyArrayAccess(ArrayAccess* expr)
	{
	}

	void ASTPolyMorpher::PolyRef(RefNode* expr)
	{
	}

	void ASTPolyMorpher::PolyDeRef(DeRefNode* expr)
	{
	}
}