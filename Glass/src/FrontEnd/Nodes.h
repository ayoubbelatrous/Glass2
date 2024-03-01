#pragma once

namespace Glass
{
	enum Tk_Type
	{
		Tk_Invalid = 0,
		Tk_Bang,
		Tk_Dollar,
		Tk_Pound,
		Tk_Period,
		Tk_SemiColon,
		Tk_Colon,
		Tk_Comma,
		Tk_Ampersand,
		Tk_Pipe,
		Tk_DoubleQoute,
		Tk_SingleQoute,
		Tk_OpenParen,
		Tk_CloseParen,
		Tk_OpenCurly,
		Tk_CloseCurly,
		Tk_OpenBracket,
		Tk_CloseBracket,
		Tk_OpenAngular,
		Tk_CloseAngular,
		Tk_GreaterEq,
		Tk_LesserEq,
		Tk_NotEqual,
		Tk_Equal,
		Tk_Plus,
		Tk_Minus,
		Tk_Star,
		Tk_Modulo,
		Tk_Slash,
		Tk_PlusAssign,
		Tk_MinusAssign,
		Tk_StarAssign,
		Tk_SlashAssign,
		Tk_BitAndAssign,
		Tk_BitOrAssign,
		Tk_Assign,
		Tk_StringLiteral,
		Tk_NumericLiteral,
		Tk_HexLiteral,
		Tk_Identifier,
		Tk_EndOfFile,

		Tk_Range,

		Tk_And,
		Tk_Or,

		Tk_If,
		Tk_Else,
		Tk_For,
		Tk_While,

		Tk_Struct,
		Tk_Enum,
		Tk_Fn,

		Tk_Return,
		Tk_Break,

		Tk_Null,
		Tk_True,
		Tk_False,
	};

	struct Tk
	{
		Tk_Type	 type;
		int			 start;
		int			 end;
		int			 line;
		String_Atom* name;
	};

	enum Ast_Node_Type
	{
		Ast_Scope,
		Ast_Variable,
		Ast_Binary,
		Ast_Numeric,
		Ast_Ident,
		Ast_Neg,
		Ast_Not,
		Ast_Ref,
		Ast_DeRef,
		Ast_Pointer,
	};

	enum Ast_Flags : u64
	{
		Ast_Flag_Declaration = BIT(0),
		Ast_Flag_Directive = BIT(1),
	};

	struct Ast_Node;

	struct Ast_Node_Variable
	{
		Ast_Node* type;
		Ast_Node* assignment;
		bool is_constant;
	};

	struct Ast_Node_Binary
	{
		Ast_Node* lhs;
		Ast_Node* rhs;
	};

	struct Ast_Node_Unary
	{
		Ast_Node* expr;
	};

	struct Ast_Node_Scope
	{
		Array_UI<Ast_Node*> stmts;
	};

	struct Ast_Node_Numeric
	{
		union
		{
			i64 integer;
			double floating;
		};

		bool is_float;
	};

	struct Ast_Node
	{
		Tk				token;
		Ast_Node_Type	type;
		Ast_Flags		flags;

		union
		{
			Ast_Node_Variable var;
			Ast_Node_Binary	  bin;
			Ast_Node_Unary	  un;
			Ast_Node_Numeric  num;
			Ast_Node_Scope    scope;
		};
	};
}