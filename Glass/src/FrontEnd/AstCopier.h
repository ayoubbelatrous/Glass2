#pragma once

#include "FrontEnd/Ast.h"

namespace Glass
{
	class ASTCopier
	{
	public:
		ASTCopier(Statement* statement);

		Statement* Copy();

		Statement* CopyStatement(Statement* stmt);

		Statement* CopyFunction(FunctionNode* func);
		Statement* CopyScope(ScopeNode* scope);
		Statement* CopyArgument(ArgumentNode* argument);
		Statement* CopyArgumentList(ArgumentList* arg_list);
		Statement* CopyVariable(VariableNode* var);

		Statement* CopyReturn(ReturnNode* ret);

		Statement* CopyIf(IfNode* ifNode);
		Statement* CopyElse(ElseNode* elseNode);
		Statement* CopyWhile(WhileNode* whil);
		Statement* CopyFor(ForNode* forNode);

		Statement* CopyTypeOf(TypeOfNode* typeof);

		Statement* CopyExpression(Expression* expr);

		Statement* CopyBinaryExpression(BinaryExpression* binExpr);
		Statement* CopyMemberAccess(MemberAccess* expr);
		Statement* CopyCallExpr(FunctionCall* expr);
		Statement* CopyTypeExpr(TypeExpression* expr);
		Statement* CopyArrayAccess(ArrayAccess* expr);

		Statement* CopyRange(RangeNode* expr);

		Statement* CopySizeOf(SizeOfNode* size_of);

		Statement* CopyCast(CastNode* cast);
		Statement* CopyAutoCast(AutoCastNode* cast);
		Statement* CopyNegate(NegateExpr* negate);
		Statement* CopyRef(RefNode* expr);
		Statement* CopyDeRef(DeRefNode* expr);

	private:
		Statement* m_Statement;
	};
}