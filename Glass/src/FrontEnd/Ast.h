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
		TypeExpression,
		Scope,
		ArgumentList,
		Function,
		Variable,
		Return,
		Call,
		StructNode,
		MemberAccess,
	};

	enum class Operator
	{
		Invalid = 0,
		Add,
		Subtract,

		Multiply,
		Divide,

		Assign
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

		// 		virtual Statement* Clone() const override {
		// 			ModuleFile* clone = Application::AllocateAstNode(*this);
		// 
		// 			for (auto stmt : m_Statements) {
		// 				clone->PushStatement(Application::AllocateAstNode(*stmt));
		// 			}
		// 
		// 			return clone;
		// 		}

		void PushStatement(Statement* stmt) {
			m_Statements.push_back(stmt);
		}

		const std::vector<Statement*>& GetStatements() const {
			return m_Statements;
		}

		virtual std::string ToString() const {
			std::string str = "Module File\n";
			for (auto stmt : m_Statements) {
				str += stmt->ToString() + '\n';
			}
			return str;
		}

		virtual const Token& GetLocation() const override {
			return Token{};
		}

	private:
		std::vector<Statement*> m_Statements;
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


	class TypeExpression : public Expression
	{
	public:

		virtual NodeType GetType() const override
		{
			return NodeType::TypeExpression;
		}

		Token Symbol;
		bool Pointer = false;
		bool Array = false;

		virtual std::string ToString() const {
			return "<" + Symbol.Symbol + ">";
		}

		virtual const Token& GetLocation() const override {
			return Symbol;
		}
	};

	class NumericLiteral : public Expression
	{
	public:

		virtual NodeType GetType() const override
		{
			return NodeType::NumericLiteral;
		}

		// 		virtual Statement* Clone() const override {
		// 			return nullptr;
		// 		}

		int Value = 0;

		Token token;

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

	class ScopeNode : public Statement
	{
	public:

		NodeType GetType() const override
		{
			return NodeType::Scope;
		}

		void PushStatement(Statement* stmt) {
			m_Statements.push_back(stmt);
		}

		const std::vector<Statement*>& GetStatements() const {
			return m_Statements;
		}

		virtual std::string ToString() const {
			std::string str = "Scope\n";
			for (auto stmt : m_Statements) {
				str += stmt->ToString() + '\n';
			}
			return str;
		}

		Token OpenCurly;
		Token CloseCurly;

		virtual const Token& GetLocation() const override {
			return OpenCurly;
		}

	private:
		std::vector<Statement*> m_Statements;
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
			for (auto arg : m_Arguments) {
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
			m_Arguments.push_back(argument);
		}

		const std::vector<Statement*>& GetArguments() const {
			return m_Arguments;
		}

		const std::vector<Statement*>& GetStatements() const {
			return m_Arguments;
		}

	private:
		std::vector<Statement*> m_Arguments;
	};

	class FunctionNode : public Statement
	{
	public:

		NodeType GetType() const override
		{
			return NodeType::Function;
		}

		void PushStatement(Statement* stmt) {
			m_Scope->PushStatement(stmt);
		}

		const std::vector<Statement*>& GetStatements() const {
			return m_Scope->GetStatements();
		}

		void SetScope(ScopeNode* scope) {
			m_Scope = scope;
		}

		ScopeNode* GetScope() {
			return m_Scope;
		}

		void SetArgList(ArgumentList* arg_list) {
			m_ArgumentList = arg_list;
		}

		const ArgumentList* GetArgList() const {
			return m_ArgumentList;
		}

		virtual std::string ToString() const {
			std::string str = "Fn " + Symbol.Symbol + " " + m_ArgumentList->ToString() + " " + " \n";
			str += m_Scope->ToString();
			return str;
		}

		Token DefinitionTk;
		Token Symbol;

		TypeExpression* ReturnType = nullptr;

		virtual const Token& GetLocation() const override {
			return Symbol;
		}

	private:
		ArgumentList* m_ArgumentList = nullptr;
		ScopeNode* m_Scope = nullptr;
	};


	class VariableNode : public Expression
	{
	public:

		virtual NodeType GetType() const override
		{
			return NodeType::Variable;
		}

		Token Symbol;
		TypeExpression* Type = nullptr;

		Expression* Assignment = nullptr;

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
		std::vector<Expression*> Arguments;

		virtual NodeType GetType() const override
		{
			return NodeType::Call;
		}

		virtual std::string ToString() const {
			return "Call " + Function.Symbol;
		}

		virtual const Token& GetLocation() const override {
			return Function;
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
}