#pragma once

#include <string>
#include <unordered_map>

namespace dynamix {

	enum class TokenType
	{
		// Single-character tokens.
		LParen, RParen,
		LBracket, RBracket,
		Comma, Dot, Minus, Plus,
		Semicolon, Slash, Star,

		// One or two character tokens.
		Bang, BangEq,
		Eq, EqEq,
		Gt, Gte,
		Lt, Lte,

		// Literals.
		Ident, String, Number, Char,

		// Keywords.
		And, Struct, Else, False,
		For, Fun, If, Null, Or,
		Print, Return, Super, Self,
		True, Let, While,

		Error, Eof
	};

	struct Token
	{
		TokenType type;
		const char* start;
		uint32_t length;
		uint32_t column;
		uint32_t line;
	};

	class Lexer
	{
	public:
		Lexer(const std::string& source);

		Token scan_token();

	private:
		Token string();
		Token number();
		Token character();
		Token identifier();

		void trim();
		void next_line();
		TokenType identifier_type() const;
		char advance();
		char peek() const;
		char peek_next() const;
		bool match(char c);
		bool is_at_end() const;

		bool is_digit(char c) const;
		bool is_alpha(char c) const;
		bool is_alnum(char c) const;

		Token make_token(TokenType type);
		Token error_token(const char* err);

	private:
		std::string m_Source;
		const char* m_Current;
		const char* m_Start;
		const char* m_LineStart;
		uint32_t m_Line;
		std::unordered_map<std::string, TokenType> m_Keywords;
	};

}
