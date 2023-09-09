#include "pch.h"

#include "FrontEnd/AstPolymorpher.h"

namespace Glass
{
	ASTPolyMorpher::ASTPolyMorpher(Statement* statement, std::unordered_map <std::string, TypeExpression*> replacements)
		:m_Statement(statement), m_Replacements(replacements)
	{
		GS_CORE_ASSERT(replacements.size() > 0);
	}

	void ASTPolyMorpher::Poly()
	{
		PolyStatement(m_Statement);
	}

	void ASTPolyMorpher::ReplaceIfMatch(const std::string& selector, TypeExpression** type)
	{
		auto it = m_Replacements.find(selector);

		if (it != m_Replacements.end()) {
			*type = it->second;
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
		case NodeType::MemberAccess:
		case NodeType::ArrayAccess:
		case NodeType::Reference:
		case NodeType::DeReference:
		case NodeType::Call:
			PolyExpression((Expression*)stmt);
			break;
		case NodeType::TypeOf: {
			TypeOfNode* type_of = (TypeOfNode*)stmt;
			PolyExpression(type_of->What);
		}
							 break;
		case NodeType::ArgumentList: {
			PolyArgumentList((ArgumentList*)stmt);
		}
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
		case NodeType::Argument:
			PolyArgument((ArgumentNode*)stmt);
			break;
		default:
			GS_CORE_ASSERT(0);
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
			return;
		}
		break;
		case NodeType::BinaryExpression:
			PolyBinaryExpression((BinaryExpression*)expr);
			return;
		case NodeType::Call:
			PolyCallExpr((FunctionCall*)expr);
			return;
		}

		GS_CORE_ASSERT(0);
	}

	void ASTPolyMorpher::PolyFunction(FunctionNode* func)
	{
		auto arg_list = func->GetArgList();

		PolyArgumentList(arg_list);

		auto& stmts = func->GetScope()->GetStatements();

		for (Statement* statement : stmts) {
			PolyStatement(statement);
		}

		if (func->ReturnType)
			PolyStatement(func->ReturnType);
	}

	void ASTPolyMorpher::PolyScope(ScopeNode* scope)
	{
		GS_CORE_ASSERT(0);
	}

	void ASTPolyMorpher::PolyArgumentList(ArgumentList* arg_list)
	{
		for (Statement* a : arg_list->GetArguments()) {
			PolyArgument((ArgumentNode*)a);
		}
	}

	void ASTPolyMorpher::PolyVariable(VariableNode* var)
	{
		PolyTypeExpr(&var->Type);
		if (var->Assignment)
			PolyStatement(var->Assignment);
	}

	void ASTPolyMorpher::PolyArgument(ArgumentNode* argument)
	{
		PolyTypeExpr(&argument->Type);
	}

	void ASTPolyMorpher::PolyReturn(ReturnNode* ret)
	{
		GS_CORE_ASSERT(0);
	}

	void ASTPolyMorpher::PolyIf(IfNode* ifNode)
	{
		GS_CORE_ASSERT(0);
	}

	void ASTPolyMorpher::PolyWhile(WhileNode* whil)
	{
		GS_CORE_ASSERT(0);
	}

	void ASTPolyMorpher::PolyBinaryExpression(BinaryExpression* binExpr)
	{
	}

	void ASTPolyMorpher::PolyMemberAccess(MemberAccess* expr)
	{
		GS_CORE_ASSERT(0);
	}

	void ASTPolyMorpher::PolyCallExpr(FunctionCall* expr)
	{
		GS_CORE_ASSERT(0);
	}

	void ASTPolyMorpher::PolyTypeExpr(TypeExpression** expr)
	{
		NodeType node_type = (*expr)->GetType();

		switch (node_type)
		{
		case NodeType::TE_TypeName: {
			TypeExpressionTypeName** as_type_name = (TypeExpressionTypeName**)expr;
			ReplaceIfMatch((*as_type_name)->Symbol.Symbol, expr);
		}
								  return;
		case NodeType::TE_Dollar: {
			TypeExpressionDollar** as_dollar = (TypeExpressionDollar**)expr;
			GS_CORE_ASSERT((*as_dollar)->TypeName->GetType() == NodeType::TE_TypeName);
			ReplaceIfMatch(((TypeExpressionTypeName*)(*as_dollar)->TypeName)->Symbol.Symbol, expr);
		}
								return;
		}

		GS_CORE_ASSERT(0);
	}
	void ASTPolyMorpher::PolyArrayAccess(ArrayAccess* expr)
	{
		GS_CORE_ASSERT(0);
	}

	void ASTPolyMorpher::PolyTypeExpression(TypeExpression** type)
	{
		NodeType node_type = (*type)->GetType();

		switch (node_type) {
		case NodeType::TE_TypeName: {
			TypeExpressionTypeName** as_type_name = (TypeExpressionTypeName**)type;
			ReplaceIfMatch((*as_type_name)->Symbol.Symbol, type);
			return;
		}
		case NodeType::TE_Pointer: {
			TypeExpressionPointer** as_pointer = (TypeExpressionPointer**)type;
			return;
		}
		case NodeType::TE_Func: {
			TypeExpressionFunc** as_func = (TypeExpressionFunc**)type;
			return;
		}
		case NodeType::TE_Dollar: {
			TypeExpressionArray** as_array = (TypeExpressionArray**)type;
			return;
		}
		}

		GS_CORE_ASSERT(0);
	}

	void ASTPolyMorpher::PolyRef(RefNode* expr)
	{
		GS_CORE_ASSERT(0);
	}

	void ASTPolyMorpher::PolyDeRef(DeRefNode* expr)
	{
		GS_CORE_ASSERT(0);
	}
}