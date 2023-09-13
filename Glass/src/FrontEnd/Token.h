#pragma once

#include <string>
#include <unordered_map>

#include "Base/Types.h"

namespace Glass
{
	enum class TokenType
	{
		Invalid = 0,

		Bang,			// '!'

		Dollar,			// '$'

		Spread,			// '...'

		Pound,			// '#'

		Period,			// '.'
		SemiColon,		// ';'
		Colon,			// ':'
		Comma,			// ','
		Ampersand,		// '&'

		Pipe,			// '|'

		DoubleQoute,	// '"'
		SingleQoute,	// '''

		OpenParen,		// '('
		CloseParen,		// ')'

		OpenCurly,		// '{'
		CloseCurly,		// '}'

		OpenBracket,	// '['
		CloseBracket,	// ']'

		OpenAngular,	// '<'
		CloseAngular,	// '>'

		GreaterEq,		// '>='
		LesserEq,		// '<='

		NotEqual,		// '!='
		Equal,			// '=='

		Add,			// '+'
		Subtract,		// '-'
		Multiply,		// '*'
		Divide,			// '/'

		Assign,			// '='

		StringLiteral,
		NumericLiteral,

		Symbol,

		Range,			// '..'

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
		Token() = default;

		Token(TokenType tokenType, const std::string_view& symbol, u64 line, u64 begin, u64 end)
			:Type(tokenType), Symbol(symbol), Line(line), Begin(begin), End(end)
		{}

		std::string Symbol;

		u64 Line;
		u64 Begin;
		u64 End;

		TokenType Type;

		std::string ToString()
		{
			return fmt::format("[ Symbol: {0} , Type : {1}]", Symbol, (u64)Type);
		}
	};
}