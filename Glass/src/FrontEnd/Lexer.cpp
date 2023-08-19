#include "pch.h"
#include "FrontEnd/Lexer.h"

#include "Application.h"

namespace Glass
{
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
		for (char c : token) {

			if (counter == 0) {
				if (c == '.') {
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

	Lexer::Lexer(const std::string& source, const fs_path& file_path)
		:m_Source(source), m_Path(file_path)
	{
	}

	std::vector<Token> Lexer::Lex()
	{
		std::vector<Token> tokens;

		u64 location = 0;
		u64 length = 0;

		u64 bol = 0;
		u64 line = 0;

		std::string accumulator;

		const std::array<char, 23> splitters =
		{
			'!',
			'#',
			'.',';',',',':',

			'&','|',

			'"','\'',

			'+','-',
			'*','/',

			'{','}',

			'(',')',
			'[',']',

			'=',
			'<', '>',
		};

		const std::unordered_map<char, TokenType> token_to_type =
		{
			{'!',TokenType::Bang},
			{'#',TokenType::Pound},
			{'$',TokenType::Dollar},

			{'.',TokenType::Period},
			{';',TokenType::SemiColon},
			{',',TokenType::Comma},
			{':',TokenType::Colon},

			{'&',TokenType::Ampersand},
			{'|',TokenType::Pipe},

			//{'"',TokenType::DoubleQoute},
			//{'\'',TokenType::SingleQoute},

			{'+',TokenType::Add},
			{'-',TokenType::Subtract},
			{'*',TokenType::Multiply},
			{'/',TokenType::Divide},

			{'=',TokenType::Assign},

			{'<',TokenType::OpenAngular},
			{'>',TokenType::CloseAngular},

			{'(',TokenType::OpenParen},
			{')',TokenType::CloseParen},

			{'{',TokenType::OpenCurly},
			{'}',TokenType::CloseCurly},

			{'[',TokenType::OpenBracket},
			{']',TokenType::CloseBracket},

			//{'\n',TokenType::BOL},
		};

		auto deduce_token_type = [&]() -> TokenType
		{
			if (begins_with_alpha_alnum(accumulator))
			{
				return TokenType::Symbol;
			}
			else {

				if (accumulator[0] == '=' || accumulator[0] == '!' || accumulator[0] == '<' || accumulator[0] == '>') {
					if (accumulator.size() > 1) {
						if (accumulator == "==") {
							return TokenType::Equal;
						}
						if (accumulator == "!=") {
							return TokenType::NotEqual;
						}
						if (accumulator == ">=") {
							return TokenType::GreaterEq;
						}
						if (accumulator == "<=") {
							return TokenType::LesserEq;
						}
					}
				}

				auto it = token_to_type.find(accumulator[0]);

				if (it != token_to_type.end()) {
					return it->second;
				}
				else if (is_valid_numeric_literal(accumulator)) {
					return TokenType::NumericLiteral;
				}
				else {
					return TokenType::Invalid;
				}
			}
		};

		auto createToken = [&]()
		{
			if (!accumulator.empty())
			{
				TokenType type = deduce_token_type();

				u64 begin = 0;

				if (type == TokenType::Symbol || type == TokenType::Invalid) {
					begin = location - bol - accumulator.size();
				}
				else {
					begin = location - bol;
				}

				if (type == TokenType::Invalid) {
					Application::FatalAbort(ExitCode::LexerError, fmt::format("Invalid Syntax At: {}:{}:{}", m_Path.string(), line + 1, begin));
				}

				tokens.emplace_back(Token{ type,
					accumulator,
					line,
					begin,
					accumulator.size() }
				);

				accumulator.clear();
			}
		};

		auto createStringLiteral = [&]()
		{
			u64 begin = 0;

			begin = location - bol;

			tokens.emplace_back(Token{ TokenType::StringLiteral,
				accumulator,
				line,
				begin,
				accumulator.size() }
			);

			accumulator.clear();
		};

		auto createEOFToken = [&]()
		{
			TokenType type = TokenType::E_OF;

			tokens.emplace_back(Token{ type,
				"EOF",
				line,
				location,
				0 }
			);
		};

		bool string_collection_mode = false;
		bool double_char_operator_mode = false;

		bool comment_mode = false;
		bool previous_slash = false;

		for (char c : m_Source)
		{
			location++;

			if (!comment_mode) {
				if (!previous_slash) {
					if (c == '/') {
						previous_slash = true;

						if (location < m_Source.size()) {

							char next = m_Source.at(location);

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

				//accumulator = '\n';
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
				if (is_valid_numeric_literal(accumulator) && c == '.') {
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

					if (c == '=' || c == '!' || c == '<' || c == '>') {
						auto next_c = m_Source[location];
						if (next_c == '=') {
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
		}

		if (!accumulator.empty()) {
			createToken();
		}

		createEOFToken();

		g_LinesProcessed += line;

		return tokens;
	}
}