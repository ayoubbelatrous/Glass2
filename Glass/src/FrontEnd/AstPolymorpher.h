#pragma once

#include "FrontEnd/Ast.h"

namespace Glass
{
	class ASTPolyMorpher
	{
	public:
		ASTPolyMorpher(Statement* statement, std::unordered_map <std::string, std::string> replacements);

		void Poly();

		void PolyStatement(Statement* stmt);

		void PolyFunction(FunctionNode* func);
		void PolyScope(ScopeNode* scope);
		void PolyArgumentList(ArgumentList* arg_list);
		void PolyVariable(VariableNode* var);

		void PolyReturn(ReturnNode* ret);

		void PolyIf(IfNode* ifNode);
		void PolyWhile(WhileNode* whil);

		void PolyExpression(Expression* expr);

		void PolyBinaryExpression(BinaryExpression* binExpr);
		void PolyMemberAccess(MemberAccess* expr);
		void PolyCallExpr(FunctionCall* expr);
		void PolyTypeExpr(TypeExpression* expr);
		void PolyArrayAccess(ArrayAccess* expr);

		void PolyRef(RefNode* expr);
		void PolyDeRef(DeRefNode* expr);

	private:

		void ReplaceIfMatch(std::string& str);

		Statement* m_Statement;
		std::unordered_map <std::string, std::string> m_Replacements;
	};

}