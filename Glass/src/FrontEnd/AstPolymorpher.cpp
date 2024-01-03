#include "pch.h"

#include "FrontEnd/AstPolymorpher.h"

namespace Glass
{
	ASTPolyMorpher::ASTPolyMorpher(Statement* statement, std::map<std::string, Expression*> replacements)
		:m_Statement(statement), m_Replacements(replacements)
	{
		GS_CORE_ASSERT(replacements.size() > 0);
	}

	void ASTPolyMorpher::Poly()
	{
		PolyStatement(m_Statement);
	}

	void ASTPolyMorpher::ReplaceIfMatch(const std::string& selector, Expression** type)
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
		case NodeType::Cast:
		case NodeType::SizeOf:
		case NodeType::AutoCast:
		case NodeType::Range:
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
		case NodeType::Else:
			PolyElse((ElseNode*)stmt);
			break;
		case NodeType::While:
			PolyWhile((WhileNode*)stmt);
			break;
		case NodeType::Argument:
			PolyArgument((ArgumentNode*)stmt);
			break;
		case NodeType::For:
			PolyFor((ForNode*)stmt);
			break;
		case NodeType::Break:
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
			Identifier* as_identifier = (Identifier*)expr;
			ReplaceIfMatch(as_identifier->Symbol.Symbol, (Expression**)&expr);
		}
		return;
		case NodeType::MemberAccess:
		{
		}
		return;
		case NodeType::NegateExpression:
		{
		}
		return;
		case NodeType::ArrayAccess:
		{
			PolyArrayAccess((ArrayAccess*)expr);
		}
		return;
		case NodeType::BinaryExpression:
			PolyBinaryExpression((BinaryExpression*)expr);
			return;
		case NodeType::Call:
			PolyCallExpr((FunctionCall*)expr);
			return;
		case NodeType::Cast:
			PolyCastExpr((CastNode*)expr);
			return;
		case NodeType::SizeOf:
			PolySizeOfExpr((SizeOfNode*)expr);
			return;
		case NodeType::NumericLiteral:
		case NodeType::StringLiteral:
			return;
		case NodeType::Reference: {
			PolyExpression(((RefNode*)expr)->What);
		}
								return;
		case NodeType::DeReference: {
			PolyExpression(((DeRefNode*)expr)->What);
		}
								  return;
		case NodeType::AutoCast: {
			PolyExpression(((AutoCastNode*)expr)->Expr);
		}
							   return;
		case NodeType::Range: {
			PolyRange(((RangeNode*)expr));
		}
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
			PolyTypeExpr(&func->ReturnType);
	}

	void ASTPolyMorpher::PolyScope(ScopeNode* scope)
	{
		for (auto stmt : scope->GetStatements()) {
			PolyStatement(stmt);
		}
	}

	void ASTPolyMorpher::PolyArgumentList(ArgumentList* arg_list)
	{
		for (Statement* a : arg_list->GetArguments()) {
			PolyArgument((ArgumentNode*)a);
		}
	}

	void ASTPolyMorpher::PolyVariable(VariableNode* var)
	{
		if (var->Type) {
			PolyTypeExpr(&var->Type);
		}

		if (var->Assignment) {
			PolyStatement(var->Assignment);
		}
	}

	void ASTPolyMorpher::PolyArgument(ArgumentNode* argument)
	{
		PolyTypeExpr(&argument->Type);
	}

	void ASTPolyMorpher::PolyReturn(ReturnNode* ret)
	{
		PolyExpression(ret->Expr);
	}

	void ASTPolyMorpher::PolyIf(IfNode* ifNode)
	{
		PolyExpression(ifNode->Condition);

		for (auto stmt : ifNode->Scope->GetStatements()) {
			PolyStatement(stmt);
		}

		if (ifNode->Else) {
			PolyStatement(ifNode->Else);
		}
	}

	void ASTPolyMorpher::PolyElse(ElseNode* elseNode)
	{
		PolyStatement(elseNode->statement);
	}

	void ASTPolyMorpher::PolyWhile(WhileNode* whil)
	{
		GS_CORE_ASSERT(0);
	}

	void ASTPolyMorpher::PolyFor(ForNode* forNode)
	{
		PolyExpression(forNode->Condition);

		for (auto stmt : forNode->Scope->GetStatements()) {
			PolyStatement(stmt);
		}
	}

	void ASTPolyMorpher::PolyBinaryExpression(BinaryExpression* binExpr)
	{
		PolyExpression(binExpr->Right);
		PolyExpression(binExpr->Left);
	}

	void ASTPolyMorpher::PolyMemberAccess(MemberAccess* expr)
	{
		GS_CORE_ASSERT(0);
	}

	void ASTPolyMorpher::PolyCallExpr(FunctionCall* expr)
	{
		if (expr->Function.Symbol == "type_info") {
			PolyTypeExpr((TypeExpression**)&expr->Arguments[0]);
			return;
		}

		for (auto arg : expr->Arguments) {
			PolyExpression(arg);
		}
	}

	void ASTPolyMorpher::PolyCastExpr(CastNode* cast)
	{
		PolyTypeExpr(&cast->Type);
		PolyExpression(cast->Expr);
	}

	void ASTPolyMorpher::PolyRange(RangeNode* range)
	{
		PolyExpression(range->Begin);
		PolyExpression(range->End);
	}

	void ASTPolyMorpher::PolySizeOfExpr(SizeOfNode* size_of)
	{
		PolyTypeExpr((TypeExpression**)&size_of->Expr);
	}

	void ASTPolyMorpher::PolyTypeExpr(TypeExpression** expr)
	{
		NodeType node_type = (*expr)->GetType();

		switch (node_type)
		{
		case NodeType::TE_TypeName: {
			TypeExpressionTypeName** as_type_name = (TypeExpressionTypeName**)expr;
			ReplaceIfMatch((*as_type_name)->Symbol.Symbol, (Expression**)expr);
		}
								  return;
		case NodeType::Identifier: {
			Identifier** as_identifier = (Identifier**)expr;
			ReplaceIfMatch((*as_identifier)->Symbol.Symbol, (Expression**)expr);
		}
								 return;
		case NodeType::TE_Dollar: {
			TypeExpressionDollar** as_dollar = (TypeExpressionDollar**)expr;
			GS_CORE_ASSERT((*as_dollar)->TypeName->GetType() == NodeType::TE_TypeName);
			ReplaceIfMatch(((TypeExpressionTypeName*)(*as_dollar)->TypeName)->Symbol.Symbol, (Expression**)expr);
		}
								return;
		case NodeType::TE_Array: {
			TypeExpressionArray** as_array = (TypeExpressionArray**)expr;
			PolyTypeExpr(&(*as_array)->ElementType);
		}
							   return;
		case NodeType::TE_Pointer: {
			TypeExpressionPointer** as_pointer = (TypeExpressionPointer**)expr;
			PolyTypeExpr(&(*as_pointer)->Pointee);
		}
								 return;
		}

		GS_CORE_ASSERT(0);
	}
	void ASTPolyMorpher::PolyArrayAccess(ArrayAccess* expr)
	{
		PolyExpression(expr->Object);
		PolyExpression(expr->Index);
	}

	void ASTPolyMorpher::PolyTypeExpression(TypeExpression** type)
	{
		NodeType node_type = (*type)->GetType();

		switch (node_type) {
		case NodeType::TE_TypeName: {
			TypeExpressionTypeName** as_type_name = (TypeExpressionTypeName**)type;
			ReplaceIfMatch((*as_type_name)->Symbol.Symbol, (Expression**)type);
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