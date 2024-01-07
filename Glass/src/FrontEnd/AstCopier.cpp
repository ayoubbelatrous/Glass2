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
		case NodeType::Cast:
		case NodeType::AutoCast:
		case NodeType::TypeOf:
			return CopyExpression((Expression*)stmt);
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
		case NodeType::Else:
			return CopyElse((ElseNode*)stmt);
			break;
		case NodeType::While:
			return CopyWhile((WhileNode*)stmt);
			break;
		case NodeType::Break:
		{
			return Application::AllocateAstNode(*(BreakNode*)stmt);
		}
		break;

		case NodeType::Argument:
			return CopyArgument((ArgumentNode*)stmt);
			break;
		case NodeType::TE_TypeName:
		case NodeType::TE_Pointer:
		case NodeType::TE_Array:
		case NodeType::TE_Func:
		case NodeType::TE_Dollar:
			return CopyTypeExpr((TypeExpression*)stmt);
			break;
		case NodeType::For:
			return CopyFor((ForNode*)stmt);
			break;
		}

		GS_CORE_ASSERT(0, "Un-Reachable");

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

	Statement* ASTCopier::CopyArgument(ArgumentNode* argument)
	{
		ArgumentNode* copy = AST(*argument);
		copy->Type = (TypeExpression*)CopyStatement(argument->Type);
		return copy;
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

		if (new_if->Else) {
			new_if->Else = (ElseNode*)CopyStatement(ifNode->Else);
		}

		return new_if;
	}

	Statement* ASTCopier::CopyElse(ElseNode* elseNode)
	{
		ElseNode* new_else = Application::AllocateAstNode(*elseNode);

		new_else->statement = CopyStatement(elseNode->statement);

		return new_else;
	}

	Statement* ASTCopier::CopyWhile(WhileNode* whil)
	{
		WhileNode* new_while = Application::AllocateAstNode(*whil);

		new_while->Condition = (Expression*)CopyExpression(whil->Condition);
		new_while->Scope = (ScopeNode*)CopyStatement(whil->Scope);

		return new_while;
	}

	Statement* ASTCopier::CopyFor(ForNode* forNode)
	{
		ForNode* new_for = AST(*forNode);

		new_for->Condition = (Expression*)CopyExpression(forNode->Condition);
		new_for->Scope = (ScopeNode*)CopyStatement(forNode->Scope);

		return new_for;
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
		case NodeType::Range:
			return CopyRange((RangeNode*)expr);
			break;
		case NodeType::SizeOf:
			return CopySizeOf((SizeOfNode*)expr);
			break;
		case NodeType::Cast:
			return CopyCast((CastNode*)expr);
			break;
		case NodeType::AutoCast:
			return CopyAutoCast((AutoCastNode*)expr);
			break;
		case NodeType::NegateExpression:
			return CopyNegate((NegateExpr*)expr);
			break;
		case NodeType::TypeOf:
			return CopyTypeOf((TypeOfNode*)expr);
			break;
		}

		GS_CORE_ASSERT(0);

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
		NodeType node_type = expr->GetType();

		switch (node_type) {
		case NodeType::TE_TypeName: {
			return AST(*(TypeExpressionTypeName*)expr);
		}
								  break;
		case NodeType::TE_Dollar: {

			TypeExpressionDollar* dollar = (TypeExpressionDollar*)expr;
			auto copy = AST(*dollar);
			copy->TypeName = (TypeExpression*)CopyStatement(dollar->TypeName);
			return copy;
		}
								break;
		case NodeType::TE_Pointer: {

			TypeExpressionPointer* pointer = (TypeExpressionPointer*)expr;
			auto copy = AST(*pointer);
			copy->Pointee = (TypeExpression*)CopyStatement(pointer->Pointee);
			return copy;
		}
								 break;
		case NodeType::TE_Array: {

			TypeExpressionArray* array = (TypeExpressionArray*)expr;
			auto copy = AST(*array);
			copy->ElementType = (TypeExpression*)CopyStatement(array->ElementType);
			return copy;
		}
							   break;
		case NodeType::TE_Func: {

			TypeExpressionFunc* func = (TypeExpressionFunc*)expr;
			auto copy = AST(*func);

			u32 i = 0;
			for (auto arg : func->Arguments) {
				func->Arguments[i] = (TypeExpression*)CopyStatement(arg);
				i++;
			}

			copy->ReturnType = (TypeExpression*)CopyStatement(func->ReturnType);
			return copy;
		}
							  break;
		}

		GS_CORE_ASSERT(0);
		return nullptr;
	}

	Statement* ASTCopier::CopyArrayAccess(ArrayAccess* expr)
	{
		ArrayAccess* new_expr = Application::AllocateAstNode(*expr);

		new_expr->Object = (Expression*)CopyExpression(expr->Object);
		new_expr->Index = (Expression*)CopyExpression(expr->Index);

		return new_expr;
	}

	Statement* ASTCopier::CopyRange(RangeNode* range)
	{
		RangeNode* new_range = AST(*range);
		new_range->Begin = (Expression*)CopyExpression(range->Begin);
		new_range->End = (Expression*)CopyExpression(range->End);
		return new_range;
	}

	Statement* ASTCopier::CopySizeOf(SizeOfNode* size_of)
	{
		SizeOfNode* new_size_of = AST(SizeOfNode());
		new_size_of->Expr = (Expression*)CopyExpression(size_of->Expr);
		return new_size_of;
	}

	Statement* ASTCopier::CopyCast(CastNode* cast)
	{
		CastNode* new_cast = AST(CastNode());
		new_cast->Expr = (Expression*)CopyExpression(cast->Expr);
		new_cast->Type = (TypeExpression*)CopyTypeExpr(cast->Type);
		return new_cast;
	}

	Statement* ASTCopier::CopyAutoCast(AutoCastNode* cast)
	{
		AutoCastNode* new_cast = AST(AutoCastNode());
		new_cast->Expr = (Expression*)CopyExpression(cast->Expr);
		return new_cast;
	}

	Statement* ASTCopier::CopyNegate(NegateExpr* negate)
	{
		NegateExpr* new_negate = AST(NegateExpr());
		new_negate->What = (Expression*)CopyExpression(negate->What);
		return new_negate;
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