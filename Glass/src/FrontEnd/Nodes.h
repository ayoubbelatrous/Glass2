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

		Tk_Arrow,

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
		Tk_Type	 	 type;
		int			 start;
		int			 end;
		int			 line;
		String_Atom* name;
	};

	enum Ast_Node_Type
	{
		Ast_Scope,
		Ast_Function,
		Ast_Struct,
		Ast_Variable,
		Ast_Binary,
		Ast_Numeric,
		Ast_String,
		Ast_Ident,
		Ast_Neg,
		Ast_Not,
		Ast_Ref,
		Ast_DeRef,
		Ast_Return,
		Ast_Pointer,
		Ast_Poly,
		Ast_Array_Type,
		Ast_Func_Type,
		Ast_Member,
		Ast_Array,
		Ast_Call,
		Ast_Directive_Library,
		Ast_Directive_Add_Library,
		Ast_Directive_Load,
		Ast_If,
		Ast_While,
		Ast_For,
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
		int scope_id;
	};

	struct Ast_Node_Member
	{
		Ast_Node* expr;
		Tk member;
	};

	struct Ast_Node_Function
	{
		Array_UI<Ast_Node*> parameters;
		Ast_Node* return_type;
		Ast_Node_Scope		body;
		String_Atom* foreign;
		bool has_body;
		bool c_varargs;
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

	struct Ast_Node_Call
	{
		Ast_Node* callee;
		Array_UI<Ast_Node*> args;
	};

	struct Ast_Node_Struct
	{
		Ast_Node_Scope		body;
	};

	struct Ast_Node_Array_Type
	{
		bool dynamic;
		Ast_Node* size;
		Ast_Node* elem;
	};

	struct Ast_Node_Func_Type
	{
		Array_UI<Ast_Node*> params;
		Ast_Node* return_type;
	};

	struct Ast_Node_Cond
	{
		Ast_Node* condition;
		Ast_Node* body;
		Ast_Node* named_iterator;
		Ast_Node* named_index;
		int scope_id;
	};

	struct Ast_Ply
	{
		Tk name;
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
			Ast_Node_Function fn;
			Ast_Node_Struct _struct;
			Ast_Node_Member	  mem;
			Ast_Node_Call	  call;
			Ast_Node_Array_Type	array_type;
			Ast_Node_Func_Type	func_type;
			Ast_Node_Cond		cond;
			Ast_Ply			poly;
		};
	};
}