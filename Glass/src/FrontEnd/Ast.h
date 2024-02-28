#pragma once

#include "Application.h"

namespace Glass
{
	enum class NodeType
	{
		None = 0,
		ModuleFile,
		Expression,
		Identifier,
		NumericLiteral,
		StringLiteral,
		BinaryExpression,

		NegateExpression,
		Not,

		TE_TypeName,
		TE_Pointer,
		TE_Array,

		TE_Dollar,

		TE_Func,

		Scope,
		ArgumentList,
		Function,
		Argument,
		Variable,
		Return,
		Call,
		StructNode,
		Foreign,
		MemberAccess,
		If,
		Else,
		While,
		For,
		Break,
		Reference,
		DeReference,
		TypeOf,
		Operator,
		Load,
		Library,
		AddLibrary,
		Cast,
		AutoCast,
		ArrayAccess,
		SizeOf,


		Range,

		Enum,
	};

	enum class Operator
	{
		Invalid = 0,
		Add,
		Subtract,

		Multiply,
		Modulo,
		Divide,

		Assign,

		AddAssign,
		SubAssign,
		MulAssign,
		DivAssign,

		BitOrAssign,
		BitAndAssign,

		Not,

		Equal,
		NotEqual,

		GreaterThan,
		LesserThan,

		GreaterThanEq,
		LesserThanEq,

		BitAnd,
		BitOr,

		And,
		Or
	};

	class Statement
	{
	public:

		virtual ~Statement()
		{}

		virtual NodeType GetType() const = 0;
		//virtual Statement* Clone() const = 0;


		virtual std::string ToString() const {
			return "";
		}

		virtual const Token& GetLocation() const = 0;
	};

	class ModuleFile : public Statement
	{
	public:

		NodeType GetType() const override
		{
			return NodeType::ModuleFile;
		}

		void PushStatement(Statement* stmt) {
			Statements.push_back(stmt);
		}

		std::vector<Statement*>& GetStatements() {
			return Statements;
		}

		virtual std::string ToString() const {
			std::string str = "Module File\n";
			for (auto stmt : Statements) {
				str += stmt->ToString() + '\n';
			}
			return str;
		}

		virtual const Token& GetLocation() const override {
			return Token{};
		}

		std::vector<Statement*> Statements;
	};

	class Program : public Statement
	{
	public:
		NodeType GetType() const override
		{
			return NodeType::ModuleFile;
		}

		void PushModule(ModuleFile* stmt) {
			m_Modules.push_back(stmt);
		}

		const std::vector<ModuleFile*>& GetModules() const {
			return m_Modules;
		}

		// 		virtual Statement* Clone() const override {
		// 			Program* clone = Application::AllocateAstNode(*this);
		// 
		// 			for (auto mod : m_Modules) {
		// 				clone->PushModule(Application::AllocateAstNode(*mod));
		// 			}
		// 
		// 			return clone;
		// 		}

		virtual std::string ToString() const {
			std::string str = "Program\n";
			for (auto mod : m_Modules) {
				str += mod->ToString() + '\n';
			}
			return str;
		}

		virtual const Token& GetLocation() const override {
			return Token{};
		}

		std::vector<ModuleFile*> m_Modules;
	};

	class Expression : public Statement
	{
	public:

		virtual NodeType GetType() const override
		{
			return NodeType::Expression;
		}

		// 		virtual Statement* Clone() const override {
		// 			return nullptr;
		// 		}
	};

	class Identifier : public Expression
	{
	public:

		virtual NodeType GetType() const override
		{
			return NodeType::Identifier;
		}

		// 		virtual Statement* Clone() const override {
		// 			return nullptr;
		// 		}

		Token Symbol;

		virtual std::string ToString() const {
			return Symbol.Symbol;
		}

		virtual const Token& GetLocation() const override {
			return Symbol;
		}
	};

	class FuncExpr : public Expression
	{
	public:

		Token Symbol;

		std::vector<Expression*> Arguments;
		Expression* ReturnType = nullptr;

		virtual NodeType GetType() const override {
			return NodeType::TE_Func;
		}

		virtual std::string ToString() const {
			return "(func)";
		}

		virtual const Token& GetLocation() const override {
			return Symbol;
		}
	};

	class PointerExpr : public Expression
	{
	public:

		Expression* Pointee = nullptr;
		u16 Indirection = 0;

		virtual NodeType GetType() const override {
			return NodeType::TE_Pointer;
		}

		virtual std::string ToString() const {
			return "";
		}

		virtual const Token& GetLocation() const override {
			return Pointee->GetLocation();
		}
	};

	class ArrayTypeExpr : public Expression
	{
	public:

		Expression* Size = nullptr;
		Expression* ElementType = nullptr;
		bool Dynamic = false;

		virtual NodeType GetType() const override {
			return NodeType::TE_Array;
		}

		virtual std::string ToString() const {
			return "";
		}

		virtual const Token& GetLocation() const override {
			return ElementType->GetLocation();
		}
	};

	class TypeExpressionDollar : public Expression
	{
	public:

		Expression* Type = nullptr;

		virtual NodeType GetType() const override {
			return NodeType::TE_Dollar;
		}

		virtual std::string ToString() const {
			return "";
		}

		virtual const Token& GetLocation() const override {
			return Type->GetLocation();
		}
	};

	class NumericLiteral : public Expression
	{
	public:

		enum class Type
		{
			Int = 0,
			Float,
			Double,
		};

		virtual NodeType GetType() const override
		{
			return NodeType::NumericLiteral;
		}

		// 		virtual Statement* Clone() const override {
		// 			return nullptr;
		// 		}

		union
		{
			double Double;
			double Float;
			u64 Int;
		} Val;

		i64 Value = 0;

		Token token;

		Type type;

		virtual std::string ToString() const {
			return token.Symbol;
		}

		virtual const Token& GetLocation() const override {
			return token;
		}
	};

	class StringLiteral : public Expression
	{
	public:

		virtual NodeType GetType() const override
		{
			return NodeType::StringLiteral;
		}

		Token Symbol;

		virtual std::string ToString() const {
			return Symbol.Symbol;
		}

		virtual const Token& GetLocation() const override {
			return Symbol;
		}
	};

	class BinaryExpression : public Expression
	{
	public:

		virtual NodeType GetType() const override
		{
			return NodeType::BinaryExpression;
		}

		// 		virtual Statement* Clone() const override {
		// 			return nullptr;
		// 		}

		Expression* Right = nullptr;
		Expression* Left = nullptr;

		Operator OPerator;
		Token OperatorToken;

		virtual std::string ToString() const {
			return "(" + Right->ToString() + OperatorToken.Symbol + Left->ToString() + ")";
		}

		virtual const Token& GetLocation() const override {
			return Left->GetLocation();
		}
	};

	class NegateExpr : public Expression
	{
	public:

		Expression* What = nullptr;

		virtual NodeType GetType() const override
		{
			return NodeType::NegateExpression;
		}

		virtual std::string ToString() const {
			return "";
		}

		virtual const Token& GetLocation() const override {
			return What->GetLocation();
		}
	};

	class NotExpr : public Expression
	{
	public:

		Expression* What = nullptr;

		virtual NodeType GetType() const override
		{
			return NodeType::NegateExpression;
		}

		virtual std::string ToString() const {
			return "";
		}

		virtual const Token& GetLocation() const override {
			return What->GetLocation();
		}
	};

	class ScopeNode : public Statement
	{
	public:

		NodeType GetType() const override
		{
			return NodeType::Scope;
		}

		void PushStatement(Statement* stmt) {
			Statements.push_back(stmt);
		}

		const std::vector<Statement*>& GetStatements() const {
			return Statements;
		}

		std::vector<Statement*>& GetStatements() {
			return Statements;
		}

		virtual std::string ToString() const {
			std::string str = "Scope\n";
			for (auto stmt : Statements) {
				str += stmt->ToString() + '\n';
			}
			return str;
		}

		Token OpenCurly;
		Token CloseCurly;

		virtual const Token& GetLocation() const override {
			return OpenCurly;
		}

		std::vector<Statement*> Statements;
	};

	class ArgumentList : public Statement
	{
	public:

		NodeType GetType() const override
		{
			return NodeType::ArgumentList;
		}

		virtual std::string ToString() const {
			std::string str = "(";
			for (auto arg : Arguments) {
				str += arg->ToString() + '\n';
			}
			return str + ")";
		}

		Token OpenParen;
		Token CloseParen;

		virtual const Token& GetLocation() const override {
			return OpenParen;
		}

		void PushArgument(Statement* argument) {
			Arguments.push_back(argument);
		}

		const std::vector<Statement*>& GetArguments() const {
			return Arguments;
		}

		std::vector<Statement*>& GetArguments() {
			return Arguments;
		}

		std::vector<Statement*> Arguments;

	};

	class ArgumentNode : public Expression
	{
	public:

		Token Symbol;
		Expression* Type = nullptr;
		bool Variadic = false;
		bool PolyMorphic = false;

		virtual NodeType GetType() const override
		{
			return NodeType::Argument;
		}

		virtual const Token& GetLocation() const override {
			return Symbol;
		}

		virtual std::string ToString() const {
			return Type->ToString() + " : " + Symbol.Symbol;
		}
	};

	class ForeignNode;

	class FunctionNode : public Statement
	{
	public:

		NodeType GetType() const override
		{
			return NodeType::Function;
		}

		void PushStatement(Statement* stmt) {
			scope->PushStatement(stmt);
		}

		const std::vector<Statement*>& GetStatements() const {
			return scope->GetStatements();
		}

		void SetScope(ScopeNode* scope) {
			this->scope = scope;
		}

		ScopeNode* GetScope() {
			return scope;
		}

		void SetArgList(ArgumentList* arg_list) {
			argumentList = arg_list;
		}

		ArgumentList* GetArgList() {
			return argumentList;
		}

		virtual std::string ToString() const {
			return "";
		}

		Token DefinitionTk;
		Token Symbol;

		Expression* ReturnType = nullptr;

		bool CVariadic = false;

		virtual const Token& GetLocation() const override {
			return Symbol;
		}

		ArgumentList* argumentList = nullptr;
		ScopeNode* scope = nullptr;
		ForeignNode* foreign_directive = nullptr;
	};


	class VariableNode : public Expression
	{
	public:

		virtual NodeType GetType() const override
		{
			return NodeType::Variable;
		}

		Token Symbol;
		Expression* Type = nullptr;

		Expression* Assignment = nullptr;

		bool Constant = false;

		virtual const Token& GetLocation() const override {
			return Symbol;
		}

		virtual std::string ToString() const {
			return Type->ToString() + " : " + Symbol.Symbol;
		}
	};

	class FunctionCall : public Expression
	{
	public:

		Token Function;
		Expression* callee = nullptr;
		std::vector<Expression*> Arguments;

		virtual NodeType GetType() const override
		{
			return NodeType::Call;
		}

		virtual std::string ToString() const {
			return "Call " + Function.Symbol;
		}

		virtual const Token& GetLocation() const override {
			return callee->GetLocation();
		}
	};

	class ReturnNode : public Statement
	{
	public:

		virtual NodeType GetType() const override
		{
			return NodeType::Return;
		}

		Token RetSymbol;
		Expression* Expr = nullptr;

		virtual std::string ToString() const {
			return "ret " + Expr->ToString();
		}
		virtual const Token& GetLocation() const override {
			return RetSymbol;
		}
	};

	class StructNode : public Statement
	{
	public:

		Token Name;
		std::vector<VariableNode*> m_Members;
		ArgumentList* argument_list = nullptr;

		virtual NodeType GetType() const override
		{
			return NodeType::StructNode;
		}

		virtual std::string ToString() const {
			return "struct";
		}
		virtual const Token& GetLocation() const override {
			return Name;
		}
	};

	class MemberAccess : public Expression
	{
	public:

		Expression* Object;
		Expression* Member;

		virtual NodeType GetType() const override
		{
			return NodeType::MemberAccess;
		}

		virtual std::string ToString() const {
			return Object->ToString() + "." + Member->ToString();
		}

		virtual const Token& GetLocation() const override {
			return Member->GetLocation();
		}
	};

	class ArrayAccess : public Expression
	{
	public:

		Expression* Object;
		Expression* Index;

		virtual NodeType GetType() const override
		{
			return NodeType::ArrayAccess;
		}

		virtual std::string ToString() const {
			return Object->ToString() + "[" + Index->ToString() + "]";
		}

		virtual const Token& GetLocation() const override {
			return Index->GetLocation();
		}
	};

	class ForeignNode : public Statement
	{
	public:

		Identifier* library_name = nullptr;
		Statement* statement = nullptr;

		virtual NodeType GetType() const override
		{
			return NodeType::Foreign;
		}

		virtual std::string ToString() const {
			return "#foreign" + statement->ToString();
		}

		virtual const Token& GetLocation() const override {
			return library_name->GetLocation();
		}
	};

	class ElseNode : public Statement
	{
	public:

		Statement* statement = nullptr;

		virtual NodeType GetType() const override
		{
			return NodeType::Else;
		}

		virtual std::string ToString() const {
			return "else";
		}

		virtual const Token& GetLocation() const override {
			return statement->GetLocation();
		}
	};

	class IfNode : public Statement
	{
	public:

		Expression* Condition = nullptr;
		ScopeNode* Scope = nullptr;
		ElseNode* Else = nullptr;

		virtual NodeType GetType() const override
		{
			return NodeType::If;
		}

		virtual std::string ToString() const {
			return "if" + Condition->ToString();
		}

		virtual const Token& GetLocation() const override {
			return Condition->GetLocation();
		}
	};

	class WhileNode : public Statement
	{
	public:

		Expression* Condition = nullptr;
		ScopeNode* Scope = nullptr;

		virtual NodeType GetType() const override
		{
			return NodeType::While;
		}

		virtual std::string ToString() const {
			return "while" + Condition->ToString();
		}

		virtual const Token& GetLocation() const override {
			return Condition->GetLocation();
		}
	};

	class ForNode : public Statement
	{
	public:

		Expression* Named_Iterator = nullptr;
		Expression* Named_Iterator_Index = nullptr;

		Expression* Condition = nullptr;
		ScopeNode* Scope = nullptr;

		virtual NodeType GetType() const override
		{
			return NodeType::For;
		}

		virtual std::string ToString() const {
			return "for " + Condition->ToString();
		}

		virtual const Token& GetLocation() const override {
			return Condition->GetLocation();
		}
	};

	class BreakNode : public Statement
	{
	public:

		Token BR;

		virtual NodeType GetType() const override
		{
			return NodeType::Break;
		}

		virtual std::string ToString() const {
			return "break";
		}

		virtual const Token& GetLocation() const override {
			return BR;
		}
	};

	class RefNode : public Statement
	{
	public:

		Expression* What;

		virtual NodeType GetType() const override
		{
			return NodeType::Reference;
		}

		virtual std::string ToString() const {
			return "& (" + What->ToString() + ") ";
		}

		virtual const Token& GetLocation() const override {
			return What->GetLocation();
		}
	};

	class DeRefNode : public Statement
	{
	public:

		Expression* What;

		virtual NodeType GetType() const override
		{
			return NodeType::DeReference;
		}

		virtual std::string ToString() const {
			return "& (" + What->ToString() + ") ";
		}

		virtual const Token& GetLocation() const override {
			return What->GetLocation();
		}
	};

	class TypeOfNode : public Expression
	{
	public:

		Expression* What;

		virtual NodeType GetType() const override
		{
			return NodeType::TypeOf;
		}

		virtual std::string ToString() const {
			return "typeof(" + What->ToString() + ")";
		}

		virtual const Token& GetLocation() const override {
			return What->GetLocation();
		}
	};

	class OperatorNode : public Statement
	{
	public:

		Statement* statement = nullptr;
		Operator OPerator;

		virtual NodeType GetType() const override
		{
			return NodeType::Operator;
		}

		virtual std::string ToString() const {
			return "#operator {TODO}" + statement->ToString();
		}

		virtual const Token& GetLocation() const override {
			return statement->GetLocation();
		}
	};

	class LibraryNode : public Statement
	{
	public:

		Token Name;
		StringLiteral* FileName = nullptr;

		virtual NodeType GetType() const override
		{
			return NodeType::Library;
		}

		virtual std::string ToString() const {
			return "#library " + FileName->ToString();
		}

		virtual const Token& GetLocation() const override {
			return FileName->GetLocation();
		}
	};

	class AddLibraryNode : public Statement
	{
	public:

		StringLiteral* FileName = nullptr;

		virtual NodeType GetType() const override
		{
			return NodeType::AddLibrary;
		}

		virtual std::string ToString() const {
			return "";
		}

		virtual const Token& GetLocation() const override {
			return FileName->GetLocation();
		}
	};

	class LoadNode : public Statement
	{
	public:

		StringLiteral* FileName = nullptr;

		virtual NodeType GetType() const override
		{
			return NodeType::Load;
		}

		virtual std::string ToString() const {
			return "#load " + FileName->ToString();
		}

		virtual const Token& GetLocation() const override {
			return FileName->GetLocation();
		}
	};

	class CastNode : public Expression
	{
	public:

		Expression* Expr;
		Expression* Type;

		virtual NodeType GetType() const override
		{
			return NodeType::Cast;
		}

		virtual std::string ToString() const {
			return "CAST " + Type->ToString();
		}

		virtual const Token& GetLocation() const override {
			return Type->GetLocation();
		}
	};

	class AutoCastNode : public Expression
	{
	public:

		Expression* Expr = nullptr;

		virtual NodeType GetType() const override
		{
			return NodeType::AutoCast;
		}

		virtual std::string ToString() const {
			return "AutoCast " + Expr->ToString();
		}

		virtual const Token& GetLocation() const override {
			return Expr->GetLocation();
		}
	};

	class EnumNode : public Expression
	{
	public:

		Token Name;
		std::vector <Identifier*> Members;

		bool Flags = false;

		virtual NodeType GetType() const override
		{
			return NodeType::Enum;
		}

		virtual std::string ToString() const {
			return "Enum";
		}

		virtual const Token& GetLocation() const override {
			return Name;
		}
	};

	class SizeOfNode : public Expression
	{
	public:

		Expression* Expr;

		virtual NodeType GetType() const override
		{
			return NodeType::SizeOf;
		}

		virtual std::string ToString() const {
			return "SizeOf";
		}

		virtual const Token& GetLocation() const override {
			return Expr->GetLocation();
		}
	};

	class RangeNode : public Expression
	{
	public:

		Expression* Begin = nullptr;
		Expression* End = nullptr;

		virtual NodeType GetType() const override
		{
			return NodeType::Range;
		}

		virtual std::string ToString() const {
			return "range .. ";
		}

		virtual const Token& GetLocation() const override {
			return Begin->GetLocation();
		}
	};
}