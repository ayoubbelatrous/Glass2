#include "pch.h"

#include "FrontEnd/AstCopier.h"

namespace Glass
{
	ASTCopier::ASTCopier(Statement* statement)
		:m_Statement(statement)
	{}

	Statement* ASTCopier::Copy()
	{
		return CopyStatement(m_Statement);
	}

	Statement* ASTCopier::CopyStatement(Statement* stmt)
	{
		if (stmt == nullptr)
			return nullptr;

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
			return CopyExpression((Expression*)stmt);
			break;
		case NodeType::TypeOf:
			return CopyTypeOf((TypeOfNode*)stmt);
			break;

		case NodeType::ArgumentList:
			return CopyArgumentList((ArgumentList*)stmt);
			break;
		case NodeType::Function:
			return CopyFunction((FunctionNode*)stmt);
			break;
		case NodeType::Scope:
			return CopyScope((ScopeNode*)stmt);
			break;

		case NodeType::Variable:
			return CopyVariable((VariableNode*)stmt);
			break;

		case NodeType::Return:
			return CopyReturn((ReturnNode*)stmt);
			break;

		case NodeType::If:
			return CopyIf((IfNode*)stmt);
			break;
		case NodeType::While:
			return CopyWhile((WhileNode*)stmt);
			break;
		case NodeType::For:
			break;
		case NodeType::Break:
		{
			return Application::AllocateAstNode(*(BreakNode*)stmt);
		}
		break;

		default:
			return stmt;
			break;
		}

		return nullptr;
	}

	Statement* ASTCopier::CopyFunction(FunctionNode* func)
	{
		FunctionNode* new_func = Application::AllocateAstNode(*func);

		new_func->ReturnType = (TypeExpression*)CopyStatement(func->ReturnType);
		new_func->SetScope((ScopeNode*)CopyStatement(func->GetScope()));
		new_func->SetArgList((ArgumentList*)CopyStatement(func->GetArgList()));
		new_func->ReturnType = (TypeExpression*)CopyStatement(func->ReturnType);
		return new_func;
	}

	Statement* ASTCopier::CopyScope(ScopeNode* scope)
	{
		ScopeNode* new_scope = Application::AllocateAstNode(ScopeNode());

		new_scope->OpenCurly = scope->OpenCurly;
		new_scope->CloseCurly = scope->CloseCurly;

		for (Statement* stmt : scope->GetStatements()) {
			new_scope->PushStatement(CopyStatement(stmt));
		}

		return new_scope;
	}

	Statement* ASTCopier::CopyArgumentList(ArgumentList* arg_list)
	{
		ArgumentList* new_arg_list = Application::AllocateAstNode(ArgumentList());

		new_arg_list->OpenParen = arg_list->OpenParen;
		new_arg_list->CloseParen = arg_list->CloseParen;

		auto& args = arg_list->GetArguments();

		for (Statement* arg : args) {
			new_arg_list->PushArgument(CopyStatement(arg));
		}

		return new_arg_list;
	}

	Statement* ASTCopier::CopyVariable(VariableNode* var)
	{
		VariableNode* new_var = Application::AllocateAstNode(*var);

		new_var->Type = (TypeExpression*)CopyStatement(var->Type);
		new_var->Assignment = (Expression*)CopyStatement(var->Assignment);

		return new_var;
	}

	Statement* ASTCopier::CopyReturn(ReturnNode* ret)
	{
		ReturnNode* new_ret = Application::AllocateAstNode(*ret);
		new_ret->Expr = (Expression*)CopyExpression(ret->Expr);
		return new_ret;
	}

	Statement* ASTCopier::CopyIf(IfNode* ifNode)
	{
		IfNode* new_if = Application::AllocateAstNode(*ifNode);

		new_if->Condition = (Expression*)CopyExpression(ifNode->Condition);
		new_if->Scope = (ScopeNode*)CopyStatement(ifNode->Scope);

		return new_if;
	}

	Statement* ASTCopier::CopyWhile(WhileNode* whil)
	{
		WhileNode* new_while = Application::AllocateAstNode(*whil);

		new_while->Condition = (Expression*)CopyExpression(new_while->Condition);
		new_while->Scope = (ScopeNode*)CopyStatement(new_while->Scope);

		return new_while;
	}

	Statement* ASTCopier::CopyTypeOf(TypeOfNode* typeof)
	{
		TypeOfNode* new_typeof = Application::AllocateAstNode(*typeof);
		new_typeof->What = (Expression*)CopyExpression(typeof->What);
		return new_typeof;
	}

	Statement* ASTCopier::CopyExpression(Expression* expr)
	{
		NodeType type = expr->GetType();

		switch (type)
		{
		case NodeType::Identifier:
		{
			return (Statement*)Application::AllocateAstNode(*(Identifier*)expr);
		}
		break;
		case NodeType::NumericLiteral:
		{
			return (Statement*)Application::AllocateAstNode(*(NumericLiteral*)expr);
		}
		break;
		case NodeType::StringLiteral:
		{
			return (Statement*)Application::AllocateAstNode(*(StringLiteral*)expr);
		}
		break;
		case NodeType::BinaryExpression:
			return CopyBinaryExpression((BinaryExpression*)expr);
			break;
		case NodeType::MemberAccess:
			return CopyMemberAccess((MemberAccess*)expr);
			break;
		case NodeType::ArrayAccess:
			return CopyArrayAccess((ArrayAccess*)expr);
			break;
		case NodeType::Reference:
			return CopyRef((RefNode*)expr);
			break;
		case NodeType::DeReference:
			return CopyDeRef((DeRefNode*)expr);
			break;
		case NodeType::Call:
			return CopyCallExpr((FunctionCall*)expr);
			break;
		}
		return nullptr;
	}

	Statement* ASTCopier::CopyBinaryExpression(BinaryExpression* binExpr)
	{
		BinaryExpression* new_expr = Application::AllocateAstNode(*binExpr);

		new_expr->Left = (Expression*)CopyExpression(binExpr->Left);
		new_expr->Right = (Expression*)CopyExpression(binExpr->Right);

		return new_expr;
	}

	Statement* ASTCopier::CopyMemberAccess(MemberAccess* expr)
	{
		MemberAccess* new_expr = Application::AllocateAstNode(*expr);

		new_expr->Object = (Expression*)CopyExpression(expr->Object);
		new_expr->Member = (Expression*)CopyExpression(expr->Member);

		return new_expr;
	}

	Statement* ASTCopier::CopyCallExpr(FunctionCall* expr)
	{
		FunctionCall* new_expr = Application::AllocateAstNode(FunctionCall());

		new_expr->Function = expr->Function;

		for (Expression* arg : expr->Arguments) {
			new_expr->Arguments.push_back((Expression*)CopyExpression(arg));
		}

		return new_expr;
	}

	Statement* ASTCopier::CopyTypeExpr(TypeExpression* expr)
	{
		//return Application::AllocateAstNode(*expr);
		return nullptr;
	}

	Statement* ASTCopier::CopyArrayAccess(ArrayAccess* expr)
	{
		ArrayAccess* new_expr = Application::AllocateAstNode(*expr);

		new_expr->Object = (Expression*)CopyExpression(expr->Object);
		new_expr->Index = (Expression*)CopyExpression(expr->Index);

		return new_expr;
	}

	Statement* ASTCopier::CopyRef(RefNode* expr)
	{
		RefNode* new_expr = Application::AllocateAstNode(*expr);
		new_expr->What = (Expression*)CopyExpression(expr->What);
		return new_expr;
	}

	Statement* ASTCopier::CopyDeRef(DeRefNode* expr)
	{
		DeRefNode* new_expr = Application::AllocateAstNode(*expr);
		new_expr->What = (Expression*)CopyExpression(expr->What);
		return new_expr;
	}
}