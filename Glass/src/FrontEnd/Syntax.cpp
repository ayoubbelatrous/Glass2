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
	String_Atom* keyword_c_varargs;
	String_Atom* keyword_foreign;
	String_Atom* keyword_library;
	String_Atom* keyword_add_library;
	String_Atom* keyword_load;

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
		keyword_c_varargs = get_atom(String_Make("c_varargs"));
		keyword_foreign = get_atom(String_Make("foreign"));
		keyword_library = get_atom(String_Make("library"));
		keyword_add_library = get_atom(String_Make("add_library"));
		keyword_load = get_atom(String_Make("load"));
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
					|| c == '+' || c == '-' || c == '*' || c == '/' || c == '&' || c == '|' || c == '.' || c == '>') {
					if (accumulator.size() > 1) {
						if (accumulator == "==")
						{
							return Tk_Equal;
						}
						else if (accumulator == "->")
						{
							return Tk_Arrow;
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
				else if (type == Tk_NumericLiteral || type == Tk_HexLiteral)
				{
					begin = location - bol - 1;
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
					if (tok.name == keyword_fn)
						type = Tk_Fn;
					if (tok.name == keyword_return)
						type = Tk_Return;
					if (tok.name == keyword_struct)
						type = Tk_Struct;
					if (tok.name == keyword_if)
						type = Tk_If;
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
						if (next_c == '=' || (c == '.' && next_c == '.') || (c == '-' && next_c == '>')) {
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
	Ast_Node* parse_binary_expr(Parser_State& s, int prec_in);
	Ast_Node* parse_statement(Parser_State& s, bool expect_semi);
	Ast_Node* parse_var_declaration(Parser_State& s, Ast_Node* type = nullptr);

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

	Ast_Node* parse_directive(Parser_State& s)
	{
		consume(s);

		if (!expected(s, Tk_Identifier))
		{
			frontend_push_error(*s.f, current(s), s.file_path, String_Make("expected identifier"));
			s.error = true;
			return nullptr;
		}

		String_Atom* name = consume(s).name;

		Ast_Node* node = allocate_node();

		if (name == keyword_library || name == keyword_add_library)
		{
			node->token = consume(s);

			if (name == keyword_library)
				node->type = Ast_Directive_Library;
			else
				node->type = Ast_Directive_Add_Library;

			if (node->token.type != Tk_StringLiteral)
			{
				frontend_push_error(*s.f, current(s), s.file_path, String_Make("expected library filename \"example\" "));
				s.error = true;
				return nullptr;
			}
		}
		else if (name == keyword_load)
		{
			node->token = consume(s);

			node->type = Ast_Directive_Load;

			if (node->token.type != Tk_StringLiteral)
			{
				frontend_push_error(*s.f, current(s), s.file_path, String_Make("expected filename"));
				s.error = true;
				return nullptr;
			}
		}
		else
		{
			frontend_push_error(*s.f, current(s), s.file_path, String_Make("unknown directive"));
			s.error = true;
			return nullptr;
		}
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
		else if (tk.type == Tk_HexLiteral)
		{
			consume(s);

			Ast_Node* lit = allocate_node();
			lit->type = Ast_Numeric;
			lit->token = tk;

			lit->num.integer = std::stoll(tk.name->str.data);

			return lit;
		}
		else if (tk.type == Tk_StringLiteral)
		{
			consume(s);

			Ast_Node* str_lit = allocate_node();
			str_lit->type = Ast_String;
			str_lit->token = tk;

			return str_lit;
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
		else if (tk.type == Tk_OpenBracket)
		{
			auto array_type = allocate_node();

			array_type->type = Ast_Array_Type;
			array_type->token = consume(s);

			if (current(s).type == Tk_Range) {
				consume(s);
				array_type->array_type.dynamic = true;
			}
			else {
				array_type->array_type.size = parse_expr(s);
			}

			if (!expected(s, Tk_CloseBracket))
			{
				frontend_push_error(*s.f, current(s), s.file_path, String_Make("Expected ']'"));
				s.error = true;
				return nullptr;
			}

			consume(s);

			Ast_Node* elem = parse_expr(s);

			if (!elem)
			{
				frontend_push_error(*s.f, current(s), s.file_path, String_Make("Expected expression"));
				s.error = true;
				return nullptr;
			}

			array_type->array_type.elem = elem;
			return array_type;
		}
		else if (tk.type == Tk_OpenParen)
		{
			bool func_type = false;

			int i = 1;
			while (current(s, i).type != Tk_EndOfFile)
			{
				if (current(s, i).type == Tk_CloseParen && current(s, i + 1).type == Tk_Arrow)
				{
					func_type = true;
					break;
				}

				i++;
			}

			auto op_paren = consume(s);

			if (func_type)
			{
				Ast_Node* func_type = allocate_node();

				func_type->type = Ast_Func_Type;
				func_type->token = op_paren;

				bool expecting_param = false;

				while (true)
				{
					if (current(s).type == Tk_CloseParen || current(s).type == Tk_EndOfFile)
					{
						if (expecting_param)
						{
							frontend_push_error(*s.f, current(s), s.file_path, String_Make("expected expression"));
							s.error = true;
							return nullptr;
						}
						break;
					}

					Ast_Node* param = parse_statement(s, false);

					Array_Add(func_type->func_type.params, param);

					if (!param)
					{
						frontend_push_error(*s.f, current(s), s.file_path, String_Make("expected expression"));
						s.error = true;
						return nullptr;
					}

					if (current(s).type == Tk_CloseParen)
					{
						break;
					}
					else
					{
						if (current(s).type == Tk_Comma)
						{
							expecting_param = true;
							consume(s);
						}
						else
						{
							frontend_push_error(*s.f, current(s), s.file_path, String_Make("expected ','"));
							s.error = true;
							return nullptr;
						}
					}
				}

				if (!expected(s, Tk_CloseParen))
				{
					frontend_push_error(*s.f, current(s), s.file_path, String_Make("expected ')'"));
					s.error = true;
					return nullptr;
				}

				consume(s);

				if (!expected(s, Tk_Arrow))
				{
					frontend_push_error(*s.f, current(s), s.file_path, String_Make("expected '->'"));
					s.error = true;
					return nullptr;
				}

				consume(s);

				func_type->func_type.return_type = parse_expr(s);

				if (!func_type->func_type.return_type)
				{
					frontend_push_error(*s.f, current(s), s.file_path, String_Make("expected expression"));
					s.error = true;
					return nullptr;
				}

				return func_type;
			}

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
		else if (tk.type == Tk_Pound)
		{
			return parse_directive(s);
		}

		return nullptr;
	}

	Ast_Node* parse_access_expr(Parser_State& s, Ast_Node* expr)
	{
		bool loop = true;

		while (loop)
		{
			switch (current(s).type)
			{
			case Tk_Period:
			{
				Tk tk_period = consume(s);

				Tk tk_member = consume(s);

				if (tk_member.type != Tk_Identifier)
				{
					frontend_push_error(*s.f, tk_member, s.file_path, String_Make("expected identifier"));
					s.error = true;
					return nullptr;
				}

				auto mem_accs = allocate_node();
				mem_accs->type = Ast_Member;
				mem_accs->token = tk_period;
				mem_accs->mem.member = tk_member;
				mem_accs->mem.expr = expr;

				expr = mem_accs;
			}
			break;
			case Tk_OpenBracket:
			{
				Tk tk_open_bracket = consume(s);

				auto arr_accs = allocate_node();
				arr_accs->type = Ast_Array;
				arr_accs->token = tk_open_bracket;
				arr_accs->bin.lhs = expr;
				arr_accs->bin.rhs = parse_expr(s);

				if (s.error) return nullptr;

				if (!arr_accs->bin.rhs)
				{
					frontend_push_error(*s.f, tk_open_bracket, s.file_path, String_Make("expected expression after '['"));
					s.error = true;
					return nullptr;
				}

				if (!expected(s, Tk_CloseBracket))
				{
					frontend_push_error(*s.f, tk_open_bracket, s.file_path, String_Make("expected closing a ']' after expression"));
					s.error = true;
					return nullptr;
				}

				consume(s);

				expr = arr_accs;
			}
			break;
			case Tk_OpenParen:
			{
				Tk tk_open_paren = consume(s);

				auto call = allocate_node();
				call->type = Ast_Call;
				call->token = tk_open_paren;
				call->call.callee = expr;
				bool expecting_param = false;

				while (true)
				{
					if (current(s).type == Tk_CloseParen || current(s).type == Tk_EndOfFile)
					{
						if (expecting_param)
						{
							frontend_push_error(*s.f, current(s), s.file_path, String_Make("expected expression"));
							s.error = true;
							return nullptr;
						}
						break;
					}

					Ast_Node* arg = parse_statement(s, false);

					Array_Add(call->call.args, arg);

					if (!arg)
					{
						frontend_push_error(*s.f, current(s), s.file_path, String_Make("expected expression"));
						s.error = true;
						return nullptr;
					}

					if (current(s).type == Tk_CloseParen)
					{
						break;
					}
					else
					{
						if (current(s).type == Tk_Comma)
						{
							expecting_param = true;
							consume(s);
						}
						else
						{
							frontend_push_error(*s.f, current(s), s.file_path, String_Make("expected ','"));
							s.error = true;
							return nullptr;
						}
					}
				}

				if (!expected(s, Tk_CloseParen))
				{
					frontend_push_error(*s.f, tk_open_paren, s.file_path, String_Make("expected a closing ')'"));
					s.error = true;
					return nullptr;
				}

				consume(s);

				expr = call;
			}
			break;
			default:
				loop = false;
				break;
			}
		}

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

	Ast_Node* parse_var_declaration(Parser_State& s, Ast_Node* type)
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

	Ast_Node* parse_statement(Parser_State& s, bool expect_semi = true);

	Ast_Node* parse_return(Parser_State& s)
	{
		Ast_Node* ret = allocate_node();

		ret->type = Ast_Return;
		ret->token = consume(s);
		ret->un.expr = parse_expr(s);

		return ret;
	}

	Ast_Node* parse_function(Parser_State& s)
	{
		consume(s);

		Ast_Node* fn = allocate_node();

		auto parse_directives = [&]()
		{
			while (current(s).type == Tk_Pound)
			{
				consume(s);

				if (current(s).type != Tk_Identifier)
				{
					frontend_push_error(*s.f, current(s), s.file_path, String_Make("expected identifier"));
					s.error = true;
					return;
				}

				if (current(s).name == keyword_foreign)
				{
					consume(s);

					if (current(s).type != Tk_Identifier)
					{
						frontend_push_error(*s.f, current(s), s.file_path, String_Make("expected library name"));
						s.error = true;
						return;
					}

					fn->fn.foreign = consume(s).name;
				}
				else if (current(s).name == keyword_c_varargs)
				{
					consume(s);
					fn->fn.c_varargs = true;
				}
				else
				{
					frontend_push_error(*s.f, current(s), s.file_path, String_Make("unsupported directive"));
					s.error = true;
					return;
				}
			}
		};

		parse_directives();
		if (s.error)
			return nullptr;

		fn->type = Ast_Function;

		if (!expected(s, Tk_Identifier))
		{
			frontend_push_error(*s.f, current(s), s.file_path, String_Make("expected function name"));
			s.error = true;
			return nullptr;
		}

		fn->token = consume(s);

		if (!expected(s, Tk_OpenParen))
		{
			frontend_push_error(*s.f, current(s), s.file_path, String_Make("expected '('"));
			s.error = true;
			return nullptr;
		}

		consume(s);

		bool expecting_param = false;

		while (true)
		{
			if (current(s).type == Tk_CloseParen || current(s).type == Tk_EndOfFile)
			{
				if (expecting_param)
				{
					frontend_push_error(*s.f, current(s), s.file_path, String_Make("expected function parameter declaration"));
					s.error = true;
					return nullptr;
				}
				break;
			}

			Ast_Node* param = parse_statement(s, false);

			Array_Add(fn->fn.parameters, param);

			if (!param)
			{
				frontend_push_error(*s.f, current(s), s.file_path, String_Make("expected function parameter declaration"));
				s.error = true;
				return nullptr;
			}

			if (param->type != Ast_Variable || param->var.is_constant)
			{
				frontend_push_error(*s.f, current(s), s.file_path, String_Make("expected function parameter to be a variable declaration"));
				s.error = true;
				return nullptr;
			}

			if (current(s).type == Tk_CloseParen)
			{
				break;
			}
			else
			{
				if (current(s).type == Tk_Comma)
				{
					expecting_param = true;
					consume(s);
				}
				else
				{
					frontend_push_error(*s.f, current(s), s.file_path, String_Make("expected ','"));
					s.error = true;
					return nullptr;
				}
			}
		}

		if (!expected(s, Tk_CloseParen))
		{
			frontend_push_error(*s.f, current(s), s.file_path, String_Make("expected ')'"));
			s.error = true;
			return nullptr;
		}

		consume(s);

		parse_directives();
		if (s.error)
			return nullptr;

		if (current(s).type == Tk_Arrow)
		{
			consume(s);

			Ast_Node* ret = parse_expr(s);

			if (!ret && !s.error)
			{
				frontend_push_error(*s.f, current(s), s.file_path, String_Make("expected expression after '->'"));
				s.error = true;
				return nullptr;
			}

			fn->fn.return_type = ret;
		}

		parse_directives();
		if (s.error)
			return nullptr;

		if (!expected(s, Tk_OpenCurly) && !expected(s, Tk_SemiColon))
		{
			frontend_push_error(*s.f, current(s), s.file_path, String_Make("expected '{' or ';'"));
			s.error = true;
			return nullptr;
		}

		Tk body_token = current(s);

		if (body_token.type == Tk_SemiColon)
		{
			fn->fn.has_body = false;
			return fn;
		}
		else
		{
			Ast_Node* body = parse_statement(s);

			if (!body) return nullptr;

			if (body->type != Ast_Scope)
			{
				frontend_push_error(*s.f, current(s), s.file_path, String_Make("expected body following function declaration"));
				s.error = true;
				return nullptr;
			}

			fn->fn.body = body->scope;

			return fn;
		}
	}

	Ast_Node* parse_struct(Parser_State& s)
	{
		consume(s);

		Ast_Node* _struct = allocate_node();
		_struct->type = Ast_Struct;
		_struct->token = consume(s);

		if (!expected(s, Tk_OpenCurly))
		{
			frontend_push_error(*s.f, current(s), s.file_path, String_Make("expected '{'"));
			s.error = true;
			return nullptr;
		}

		Ast_Node* body = parse_statement(s);

		if (!body) return nullptr;

		if (body->type != Ast_Scope)
		{
			frontend_push_error(*s.f, current(s), s.file_path, String_Make("expected body following struct declaration"));
			s.error = true;
			return nullptr;
		}

		_struct->_struct.body = body->scope;

		return _struct;
	}

	Ast_Node* parse_statement(Parser_State& s, bool expect_semi)
	{
		auto expect_semicolon = [&]()
		{
			if (s.error)
				return nullptr;

			if (expect_semi && !expected(s, Tk_SemiColon))
			{
				frontend_push_error(*s.f, current(s), s.file_path, String_Make("expected semicolon ';'"));
				s.error = true;
				return nullptr;
			}

			if (expect_semi)
				consume(s);

		};

		if (current(s).type == Tk_SemiColon)
		{
			consume(s);
			return parse_statement(s, false);
		}

		if (current(s).type == Tk_Identifier)
		{
			bool declaration = current(s, 1).type == Tk_Colon;

			if (declaration)
			{
				auto var_decl = parse_var_declaration(s);

				expect_semicolon();

				return var_decl;
			}
		}

		if (current(s).type == Tk_Return)
		{
			Ast_Node* ret = parse_return(s);

			expect_semicolon();

			return ret;
		}

		if (current(s).type == Tk_Fn)
		{
			return parse_function(s);
		}

		if (current(s).type == Tk_Struct)
		{
			return parse_struct(s);
		}

		if (current(s).type == Tk_OpenCurly)
		{
			auto scope = allocate_node();
			scope->type = Ast_Scope;
			scope->token = consume(s);

			while (true)
			{
				if (current(s).type == Tk_CloseCurly || current(s).type == Tk_EndOfFile) {
					break;
				}

				auto stmt = parse_statement(s);

				if (s.error)
				{
					return nullptr;
				}

				if (stmt)
					Array_Add(scope->scope.stmts, stmt);
			}

			if (!expected(s, Tk_CloseCurly))
			{
				frontend_push_error(*s.f, current(s), s.file_path, String_Make("expected '}'"));
				s.error = true;
				return nullptr;
			}

			consume(s);

			return scope;
		}

		auto expr = parse_expr(s);

		if (expr && current(s).type == Tk_Identifier)
		{
			expr = parse_var_declaration(s, expr);
		}

		expect_semicolon();

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
			if (s.error)
			{
				success = false;
				return nullptr;
			}

			auto stmt = parse_statement(s);

			if (s.error)
			{
				success = false;
				return nullptr;
			}

			if (stmt)
			{
				Array_Add(statements, stmt);
			}
		}

		Ast_Node* file_node = allocate_node();

		file_node->type = Ast_Scope;
		file_node->scope.stmts = *(Array_UI<Ast_Node*>*) & statements;

		success = true;
		return file_node;
	}
}
