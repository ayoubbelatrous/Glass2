#pragma once

#include <string>
#include <unordered_map>

#include "Base/Types.h"

namespace Glass
{
	enum class TokenType
	{
		Invalid = 0,

		SemiColon,		// ';'
		Colon,			// ':'
		Comma,			// ','
		Ampersand,		// '&'

		DoubleQoute,	// '"'
		SingleQoute,	// '''

		OpenParen,		// '('
		CloseParen,		// ')'

		OpenCurly,		// '{'
		CloseCurly,		// '}'

		OpenBracket,	// '['
		CloseBracket,	// ']'

		Add,			// '+'
		Subtract,		// '-'
		Multiply,		// '*'
		Divide,			// '/'

		Assign,			// '='

		StringLiteral,
		NumericLiteral,

		Symbol,

		E_OF,
		BOL,
	};

	inline std::unordered_map<TokenType, const std::string> TokenTypeStringMap =
	{
		{TokenType::Comma,","},
		{TokenType::SemiColon,";"}
	};

	struct Token
	{
		Token(TokenType tokenType, const std::string_view& symbol, u64 line, u64 begin, u64 end)
			:Type(tokenType), Symbol(symbol), Line(line), Begin(begin), End(end)
		{}

		const TokenType Type;
		const std::string Symbol;

		const u64 Line;

		const u64 Begin;
		const u64 End;

		std::string ToString()
		{
			return fmt::format("[ Symbol: {0} , Type : {1}]", Symbol, (u64)Type);
		}
	};
}