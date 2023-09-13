#pragma once

#include "FrontEnd/Ast.h"

namespace Glass
{
	class ASTPolyMorpher
	{
	public:
		ASTPolyMorpher(Statement* statement, std::map<std::string, Expression*> replacements);

		void Poly();

		void PolyStatement(Statement* stmt);

		void PolyFunction(FunctionNode* func);
		void PolyScope(ScopeNode* scope);
		void PolyArgumentList(ArgumentList* arg_list);
		void PolyVariable(VariableNode* var);
		void PolyArgument(ArgumentNode* argument);

		void PolyReturn(ReturnNode* ret);

		void PolyIf(IfNode* ifNode);
		void PolyWhile(WhileNode* whil);

		void PolyExpression(Expression* expr);

		void PolyBinaryExpression(BinaryExpression* binExpr);
		void PolyMemberAccess(MemberAccess* expr);
		void PolyCallExpr(FunctionCall* expr);

		void PolyCastExpr(CastNode* cast);

		void PolySizeOfExpr(SizeOfNode* size_of);

		void PolyTypeExpr(TypeExpression** expr);
		void PolyArrayAccess(ArrayAccess* expr);

		void PolyTypeExpression(TypeExpression** type);

		void PolyRef(RefNode* expr);
		void PolyDeRef(DeRefNode* expr);

	private:

		void ReplaceIfMatch(const std::string& selector, Expression** type);

		Statement* m_Statement;
		std::map <std::string, Expression*> m_Replacements;
	};

}