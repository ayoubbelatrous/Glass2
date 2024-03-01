#include "pch.h"

#include "FrontEnd/Syntax.h"
#include "FrontEnd/FrontEnd.h"
#include "Base/Allocator.h"

namespace Glass
{
	String_Atom* keyword_and;
	String_Atom* keyword_or;
	String_Atom* keyword_if;
	String_Atom* keyword_else;
	String_Atom* keyword_for;
	String_Atom* keyword_while;
	String_Atom* keyword_struct;
	String_Atom* keyword_enum;
	String_Atom* keyword_fn;
	String_Atom* keyword_return;
	String_Atom* keyword_break;
	String_Atom* keyword_null;
	String_Atom* keyword_true;
	String_Atom* keyword_false;

	void parser_init()
	{
		keyword_and = get_atom(String_Make("and"));
		keyword_or = get_atom(String_Make("or"));
		keyword_if = get_atom(String_Make("if"));
		keyword_else = get_atom(String_Make("else"));
		keyword_for = get_atom(String_Make("for"));
		keyword_while = get_atom(String_Make("while"));
		keyword_struct = get_atom(String_Make("struct"));
		keyword_enum = get_atom(String_Make("enum"));
		keyword_fn = get_atom(String_Make("fn"));
		keyword_return = get_atom(String_Make("return"));
		keyword_break = get_atom(String_Make("break"));
		keyword_null = get_atom(String_Make("null"));
		keyword_true = get_atom(String_Make("true"));
		keyword_false = get_atom(String_Make("false"));
	}

	inline static bool begins_with_alpha_alnum(const std::string_view& token)
	{
		if (token.size() == 0)
		{
			return false;
		}

		if (std::isalpha(token[0]) || token[0] == '_')
		{
			for (const auto& c : token)
			{
				if (!std::isalnum(c) && c != '_')
				{
					return false;
				}
			}
			return true;
		}
		else
		{
			return false;
		}
	}

	inline static bool is_valid_numeric_literal(const std::string_view& token)
	{
		u64 counter = 0;

		if (token.size() == 0) {
			return false;
		}

		for (char c : token) {

			if (counter == 0) {
				if (c == '.') {
					return false;
				}
			}

			if (c == '.') {
				if (counter + 1 < token.size()) {
					if (token[counter + 1] == '.') {
						return false;
					}
				}
				else {
					return false;
				}
			}

			if (std::isalpha(c)) {
				return false;
			}

			counter++;
		}

		return true;
	}

	inline static bool is_valid_hex_literal(const std::string_view& token)
	{
		if (token.size() < 2) {
			return false;
		}

		if (token[0] != '0' || token[1] != 'x') {
			return false;
		}

		for (size_t i = 0; i < token.size(); i++)
		{
			if (!std::isalnum(token[i])) {
				return false;
			}
		}


		return true;
	}

	Array<Tk> tokenize_string(Front_End& f, String source, String file_path, bool& success)
	{
		Array<Tk> tokens;

		u64 location = 0;
		u64 length = 0;

		u64 bol = 0;
		u64 line = 0;

		std::string accumulator;

		success = true;

		const std::unordered_map<char, Tk_Type> token_to_type =
		{
			{'!',Tk_Bang},
			{'#',Tk_Pound},
			{'$',Tk_Dollar},
			{'.',Tk_Period},
			{';',Tk_SemiColon},
			{',',Tk_Comma},
			{':',Tk_Colon},
			{'&',Tk_Ampersand},
			{'|',Tk_Pipe},
			{'+',Tk_Plus},
			{'-',Tk_Minus},
			{'*',Tk_Star},
			{'%',Tk_Modulo},
			{'/',Tk_Slash},
			{'=',Tk_Assign},
			{'<',Tk_OpenAngular},
			{'>',Tk_CloseAngular},
			{'(',Tk_OpenParen},
			{')',Tk_CloseParen},
			{'{',Tk_OpenCurly},
			{'}',Tk_CloseCurly},
			{'[',Tk_OpenBracket},
			{']',Tk_CloseBracket},
		};

		auto deduce_token_type = [&]() -> Tk_Type
		{
			if (begins_with_alpha_alnum(accumulator))
			{
				return Tk_Identifier;
			}
			else {

				auto c = accumulator[0];

				if (c == '=' || c == '!' || c == '<' || c == '>'
					|| c == '+' || c == '-' || c == '*' || c == '/' || c == '&' || c == '|' || c == '.') {
					if (accumulator.size() > 1) {
						if (accumulator == "==")
						{
							return Tk_Equal;
						}
						else if (accumulator == "..")
						{
							return Tk_Range;
						}
						else if (accumulator == "!=")
						{
							return Tk_NotEqual;
						}
						else if (accumulator == ">=")
						{
							return Tk_GreaterEq;
						}
						else if (accumulator == "<=")
						{
							return Tk_LesserEq;
						}

						else if (accumulator == "+=")
						{
							return Tk_PlusAssign;
						}
						else if (accumulator == "-=")
						{
							return Tk_MinusAssign;
						}
						else if (accumulator == "*=")
						{
							return Tk_StarAssign;
						}
						else if (accumulator == "/=")
						{
							return Tk_SlashAssign;
						}

						else if (accumulator == "&=")
						{
							return Tk_BitAndAssign;
						}
						else if (accumulator == "|=")
						{
							return Tk_BitOrAssign;
						}
					}
				}

				auto it = token_to_type.find(c);

				if (it != token_to_type.end()) {
					return it->second;
				}
				else if (is_valid_numeric_literal(accumulator)) {
					return Tk_NumericLiteral;
				}
				else if (is_valid_hex_literal(accumulator)) {
					return Tk_HexLiteral;
				}
				else {
					return Tk_Invalid;
				}
			}
		};

		auto createToken = [&]()
		{
			if (!accumulator.empty())
			{
				Tk_Type type = deduce_token_type();

				u64 begin = 0;

				if (type == Tk_Identifier || type == Tk_Invalid) {
					begin = location - bol - accumulator.size();
				}
				else {
					begin = location - bol;
				}

				if (type == Tk_Invalid) {
					frontend_push_error(f, String_Make(fmt::format("invalid syntax at: {}:{}:{}", file_path, line + 1, begin)));
					success = false;
					return;
				}

				Tk tok = {};

				if (type == Tk_Identifier || type == Tk_NumericLiteral || type == Tk_StringLiteral || type == Tk_HexLiteral)
				{
					tok.name = get_atom(String_Make(accumulator));
				}

				if (type == Tk_Identifier)
				{
					if (tok.name == keyword_and)
						type = Tk_And;
					if (tok.name == keyword_or)
						type = Tk_Or;
				}

				tok.type = type;
				tok.line = (int)line;
				tok.start = (int)begin;
				tok.end = (int)accumulator.size();

				if (type == Tk_StringLiteral || type == Tk_NumericLiteral || type == Tk_HexLiteral)
				{
					tok.start -= accumulator.size() - 1;
				}

				Array_Add(tokens, tok);

				accumulator.clear();
			}
		};

		auto createStringLiteral = [&]()
		{
			u64 begin = 0;

			begin = location - bol;

			Tk tok = {};
			tok.type = Tk_StringLiteral;
			tok.line = (int)line;
			tok.start = (int)begin - accumulator.size();
			tok.end = (int)accumulator.size();
			tok.name = get_atom(String_Make(accumulator));

			Array_Add(tokens, tok);

			accumulator.clear();
		};

		auto createEOFToken = [&]()
		{
			TokenType type = TokenType::E_OF;

			Tk tok;
			tok.type = Tk_EndOfFile;
			tok.line = (int)line;
			tok.start = (int)location;
			tok.end = 0;

			Array_Add(tokens, tok);
		};

		bool string_collection_mode = false;
		bool double_char_operator_mode = false;

		bool comment_mode = false;
		bool previous_slash = false;

		for (; location < source.count; )
		{
			char c = source[location];

			location++;

			if (!success)
				return {};

			if (!comment_mode && !string_collection_mode) {
				if (!previous_slash) {
					if (c == '/') {
						previous_slash = true;

						if (location < source.count) {

							char next = source[location];

							if (next == '/') {
								continue;
							}
						}
					}
				}
				else {
					if (c == '/') {
						comment_mode = true;
						previous_slash = false;
						continue;
					}
					else {
						previous_slash = false;
					}
				}
			}

			if (comment_mode) {
				if (c == '\n') {
					line++;
					comment_mode = false;
				}

				continue;
			}

			if (string_collection_mode) {
				if (c == '"') {
					createStringLiteral();
					string_collection_mode = false;
				}
				else
				{
					accumulator += c;
				}

				continue;
			}

			if (std::isspace(c)) {
				createToken();
			}

			if (c == '\n') {

				line++;

				bol = location;

				if (!accumulator.empty()) {
					createToken();
				}
			}

			if (std::isalnum(c) || c == '_') {
				accumulator.push_back(c);
			}

			if (c == '"' && !string_collection_mode) {
				if (!accumulator.empty()) {
					createToken();
				}
				string_collection_mode = true;
			}
			else {
				if ((is_valid_numeric_literal(accumulator) || is_valid_hex_literal(accumulator)) && c == '.' && source[location] != '.') {
					accumulator += c;
				}
				else if (token_to_type.find(c) != token_to_type.end()) {

					if (double_char_operator_mode) {
						accumulator.push_back(c);
						createToken();

						double_char_operator_mode = false;
					}
					else {
						if (!accumulator.empty()) {
							createToken();
						}

						accumulator.push_back(c);
					}

					if (c == '=' || c == '!' || c == '<' || c == '>' || c == '+' || c == '-' || c == '*' || c == '/' || c == '&' || c == '|' || c == '.') {
						auto next_c = source[location];
						if (next_c == '=' || (c == '.' && next_c == '.')) {
							double_char_operator_mode = true;
						}
						else {
							createToken();
						}
					}
					else {
						createToken();
					}
				}
			}

			if (!success)
				return {};
		}

		if (!accumulator.empty()) {
			createToken();
		}

		createEOFToken();

		success = true;

		return tokens;
	}

	LinearAllocator allocator = LinearAllocator(1024 * 1024 * 10);

	Ast_Node* parse_expr(Parser_State& s);
	Ast_Node* parse_binary_expr(Parser_State& s, int prec_in);

	Ast_Node* allocate_node()
	{
		return (Ast_Node*)allocator.Allocate_Bytes(sizeof(Ast_Node));
	}

	Tk& current(Parser_State& s, u64 ahead = 0)
	{
		return s.tokens[s.location + ahead];
	}

	Tk& consume(Parser_State& s)
	{
		s.location++;
		return s.tokens[s.location - 1];
	}

	bool expected(Parser_State& s, Tk_Type type)
	{
		return type == current(s).type;
	}

	Ast_Node* parse_expr(Parser_State& s)
	{
		return parse_binary_expr(s, 1);
	}

	int token_precedence(Tk_Type tk_type)
	{
		switch (tk_type)
		{
		case Tk_Range:
			return 1;
		case Tk_PlusAssign:
		case Tk_MinusAssign:
		case Tk_StarAssign:
		case Tk_SlashAssign:
		case Tk_BitAndAssign:
		case Tk_BitOrAssign:
		case Tk_Assign:
			return 2;
		case Tk_And:
		case Tk_Or:
			return 3;
		case Tk_Ampersand:
		case Tk_Pipe:
			return 4;
		case Tk_OpenAngular:
		case Tk_CloseAngular:
		case Tk_GreaterEq:
		case Tk_LesserEq:
		case Tk_NotEqual:
		case Tk_Equal:
			return 5;
		case Tk_Plus:
		case Tk_Minus:
			return 6;
		case Tk_Star:
		case Tk_Slash:
		case Tk_Modulo:
			return 7;
			break;
		}

		return 0;
	}

	Ast_Node* parse_primary_expr(Parser_State& s)
	{
		Tk tk = current(s);

		if (tk.type == Tk_NumericLiteral)
		{
			consume(s);

			Ast_Node* lit = allocate_node();
			lit->type = Ast_Numeric;
			lit->token = tk;

			for (size_t i = 0; i < tk.name->str.count; i++)
				if (tk.name->str[i] == '.') {
					lit->num.is_float = true;
					break;
				}

			if (lit->num.is_float)
				lit->num.floating = std::stod(tk.name->str.data);
			else
				lit->num.integer = std::stoll(tk.name->str.data);

			return lit;
		}
		else if (tk.type == Tk_Identifier)
		{
			consume(s);

			Ast_Node* ident = allocate_node();
			ident->type = Ast_Ident;
			ident->token = tk;

			return ident;
		}
		else if (tk.type == Tk_Star)
		{
			auto pointer = allocate_node();
			pointer->token = consume(s);
			pointer->type = Ast_Pointer;
			Ast_Node* pointee = parse_expr(s);
			pointer->un.expr = pointee;
			return pointer;
		}
		else if (tk.type == Tk_OpenParen)
		{
			auto op_paren = consume(s);
			auto expr = parse_expr(s);
			consume(s);

			if (!expr)
			{
				frontend_push_error(*s.f, op_paren, s.file_path, String_Make("Expected expression"));
				s.error = true;
				return nullptr;
			}

			return expr;
		}

		return nullptr;
	}

	Ast_Node* parse_access_expr(Parser_State& s, Ast_Node* expr)
	{
		return expr;
	}

	Ast_Node* parse_unary_expr(Parser_State& s)
	{
		Tk tk = current(s);

		switch (tk.type)
		{
		case Tk_Bang:
		case Tk_Minus:
		case Tk_Ampersand:
		case Tk_OpenAngular:
		{
			Tk op = consume(s);

			auto unary = allocate_node();

			unary->token = op;

			if (op.type == Tk_Minus)
				unary->type = Ast_Neg;
			else if (op.type == Tk_Bang)
				unary->type = Ast_Not;
			else if (op.type == Tk_Ampersand)
				unary->type = Ast_Ref;
			else if (op.type == Tk_OpenAngular)
				unary->type = Ast_DeRef;

			unary->un.expr = parse_unary_expr(s);

			if (!unary->un.expr && !s.error)
			{
				frontend_push_error(*s.f, op, s.file_path, String_Make("Expected expression"));
				s.error = true;
				return nullptr;
			}

			return unary;
		}
		}

		Ast_Node* expr = parse_access_expr(s, parse_primary_expr(s));
		return expr;
	}

	Ast_Node* parse_binary_expr(Parser_State& s, int prec_in)
	{
		Ast_Node* expr = parse_unary_expr(s);

		while (true) {

			Tk op = current(s);

			int op_prec = token_precedence(op.type);

			if (op_prec < prec_in) {
				break;
			}

			consume(s);

			Ast_Node* right = parse_binary_expr(s, op_prec + 1);

			if (right == nullptr) {
				frontend_push_error(*s.f, op, s.file_path, String_Make("Expected expression on the right-hand side of the binary operator 'todo'"));
				s.error = true;
				return nullptr;
			}

			Ast_Node* bin_expr = allocate_node();

			bin_expr->type = Ast_Binary;
			bin_expr->token = op;
			bin_expr->bin.lhs = expr;
			bin_expr->bin.rhs = right;

			expr = bin_expr;
		}

		return expr;
	}

	Ast_Node* parse_var_declaration(Parser_State& s, Ast_Node* type = nullptr)
	{
		if (!expected(s, Tk_Identifier))
		{
			frontend_push_error(*s.f, current(s), s.file_path, String_Make("expected identifier"));
			return nullptr;
		}

		Tk name = consume(s);

		Tk assign_tk;

		bool constant_decl = false;
		bool has_assignment = false;

		if (current(s).type == Tk_Colon)
		{
			consume(s);

			if (current(s).type == Tk_Assign)
			{
				assign_tk = consume(s);

				has_assignment = true;

				if (type)
				{
					frontend_push_error(*s.f, type->token, s.file_path, String_Make("type not expected"));
					s.error = true;
					return nullptr;
				}
			}
			else if (current(s).type == Tk_Colon)
			{
				assign_tk = consume(s);
				constant_decl = true;
				has_assignment = true;
			}
		}
		else if (current(s).type == Tk_Assign)
		{
			assign_tk = consume(s);
			has_assignment = true;
		}
		else
		{
			if (!type)
			{
				frontend_push_error(*s.f, current(s), s.file_path, String_Make("expected '::' or ':='"));
				s.error = true;
				return nullptr;
			}
		}

		Ast_Node* var = allocate_node();
		var->type = Ast_Variable;
		var->token = name;

		if (has_assignment)
		{
			var->var.assignment = parse_expr(s);

			if (!var->var.assignment)
			{
				if (s.error)
					return nullptr;

				frontend_push_error(*s.f, assign_tk, s.file_path, String_Make("expected assignment"));
				s.error = true;
			}
		}

		var->var.is_constant = constant_decl;
		var->var.type = type;

		return var;
	}

	Ast_Node* parse_statement(Parser_State& s)
	{
		if (current(s).type == Tk_Identifier)
		{
			bool declaration = current(s, 1).type == Tk_Colon;

			if (declaration)
			{
				return parse_var_declaration(s);
			}

		}

		auto expr = parse_expr(s);

		if (expr && current(s).type == Tk_Identifier)
		{
			expr = parse_var_declaration(s, expr);
		}

		if (!expected(s, Tk_SemiColon))
		{
			frontend_push_error(*s.f, current(s), s.file_path, String_Make("expected semicolon ';'"));
			s.error = true;
			return nullptr;
		}

		consume(s);

		return expr;
	}

	Ast_Node* parse_string(Front_End& f, String file_path, String source, bool& success)
	{
		Array<Tk> tokens = tokenize_string(f, source, file_path, success);

		if (!success)
			return nullptr;

		Parser_State s;
		s.location = 0;
		s.tokens = tokens;
		s.file_path = file_path;
		s.f = &f;

		Array<Ast_Node*> statements;

		while (s.location != tokens.count && current(s).type != Tk_EndOfFile)
		{
			auto stmt = parse_statement(s);

			if (s.error)
			{
				success = false;
				return nullptr;
			}

			if (stmt)
				Array_Add(statements, stmt);
		}

		Ast_Node* file_node = allocate_node();

		file_node->type = Ast_Scope;
		file_node->scope.stmts = *(Array_UI<Ast_Node*>*) & statements;

		success = true;
		return file_node;
	}
}
