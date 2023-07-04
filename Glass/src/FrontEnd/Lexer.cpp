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

		const std::array<char, 16> splitters =
		{
			';',',',':',

			'&',

			'"','\'',

			'+','-',
			'*','/',

			'{','}',

			'(',')',
			'[',']',
		};

		const std::unordered_map<char, TokenType> token_to_type =
		{
			{';',TokenType::SemiColon},
			{',',TokenType::Comma},
			{':',TokenType::Colon},

			{'&',TokenType::Ampersand},

			{'"',TokenType::DoubleQoute},
			{'\'',TokenType::SingleQoute},

			{'+',TokenType::Add},
			{'-',TokenType::Subtract},
			{'*',TokenType::Multiply},
			{'/',TokenType::Divide},

			{'=',TokenType::Assign},

			{'(',TokenType::OpenParen},
			{')',TokenType::CloseParen},

			{'{',TokenType::OpenCurly},
			{'}',TokenType::CloseCurly},

			{'[',TokenType::OpenBracket},
			{']',TokenType::CloseBracket},

			{'\n',TokenType::BOL},
		};

		auto deduce_token_type = [&]() -> TokenType
		{
			if (begins_with_alpha_alnum(accumulator))
			{
				return TokenType::Symbol;
			}
			else {


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

		for (char c : m_Source)
		{
			location++;

			if (std::isspace(c)) {
				createToken();
			}

			if (c == '\n') {

				line++;

				bol = location;

				if (!accumulator.empty()) {
					createToken();
				}

				accumulator = '\n';
			}

			if (std::isalnum(c)) {
				accumulator.push_back(c);
			}

			if (token_to_type.find(c) != token_to_type.end()) {

				if (!accumulator.empty()) {
					createToken();
				}

				accumulator.push_back(c);
				createToken();
			}
		}

		createEOFToken();

		return tokens;
	}
}

