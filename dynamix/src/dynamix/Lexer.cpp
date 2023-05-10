#include "Lexer.h"

#include <format>

namespace dynamix {
	
	Lexer::Lexer(const std::string& source)
		:
		m_Source(source),
		m_Start(source.c_str()),
		m_Current(source.c_str()),
		m_Line(1),
		m_LineStart(m_Current),
		m_Keywords({
			{ "&&",     TokenType::And    },
			{ "struct", TokenType::Struct },
			{ "else",   TokenType::Else   },
			{ "false",  TokenType::False  },
			{ "for",    TokenType::For    },
			{ "fun",    TokenType::Fun    },
			{ "if",     TokenType::If     },
			{ "null",   TokenType::Null   },
			{ "||",     TokenType::Or     },
			{ "print",  TokenType::Print  },
			{ "return", TokenType::Return },
			{ "super",  TokenType::Super  },
			{ "self",   TokenType::Self   },
			{ "true",   TokenType::True   },
			{ "let",    TokenType::Let    },
			{ "while",  TokenType::While  },
		}) { }

	Token Lexer::scan_token()
	{
		trim();

		m_Start = m_Current;

		if (is_at_end()) {
			return make_token(TokenType::Eof);
		}

		char c = advance();
		if (is_digit(c)) {
			return number();
		}
		else if (is_alpha(c)) {
			return identifier();
		}

		switch (c) {
			case '(': return make_token(TokenType::LParen);
			case ')': return make_token(TokenType::RParen);
			case '{': return make_token(TokenType::LBracket);
			case '}': return make_token(TokenType::RBracket);
			case ';': return make_token(TokenType::Semicolon);
			case ',': return make_token(TokenType::Comma);
			case '.': return make_token(TokenType::Dot);
			case '-': return make_token(TokenType::Minus);
			case '+': return make_token(TokenType::Plus);
			case '/': return make_token(TokenType::Slash);
			case '*': return make_token(TokenType::Star);
			case '!': return make_token(match('=') ? TokenType::BangEq : TokenType::Bang);
			case '=': return make_token(match('=') ? TokenType::EqEq : TokenType::Eq);
			case '<': return make_token(match('=') ? TokenType::Lte : TokenType::Lt);
			case '>': return make_token(match('=') ? TokenType::Gte : TokenType::Gt);
			case '"': return string();
			case '\'': return character();
		}

		std::string error = std::format("Unexpected character '{}'", m_Start);
		return error_token(error.c_str());
	}

	Token Lexer::string()
	{
		while (peek() != '"' && !is_at_end()) {
			if (peek() == '\n') {
				next_line();
			}
			else {
				advance();
			}
		}

		if (is_at_end()) {
			return error_token("Unterminated string literal");
		}

		advance();
		return make_token(TokenType::String);
	}

	Token Lexer::number()
	{
		while (is_digit(peek()))
			advance();

		if (peek() == '.' && is_digit(peek_next())) {
			advance();

			while (is_digit(peek()))
				advance();
		}

		return make_token(TokenType::Number);
	}

	Token Lexer::character()
	{
		m_Start++;
		Token token = make_token(TokenType::Char);
		advance();
		return token;
	}

	Token Lexer::identifier()
	{
		while (is_alnum(peek()))
			advance();

		return make_token(identifier_type());
	}

	void Lexer::trim()
	{
		for (;;) {
			char c = peek();
			switch (c) {
				case ' ':
				case '\r':
				case '\t':
					advance();
					break;
				case '/':
					if (peek_next() == '/') {
						while (peek() != '\n' && !is_at_end())
							advance();
					}
					else return;
				case '\n':
					next_line();
					break;
				default:
					return;
			}
		}
	}

	void Lexer::next_line()
	{
		m_Line++;
		advance();
		m_LineStart = &m_Current[-1];
	}

	TokenType Lexer::identifier_type() const
	{
		std::string identifier(m_Start, m_Current);

		if (m_Keywords.contains(identifier)) {
			return m_Keywords.at(identifier);
		}

		return TokenType::Ident;
	}

	char Lexer::advance()
	{
		m_Current++;
		return m_Current[-1];
	}

	char Lexer::peek() const
	{
		return *m_Current;
	}

	char Lexer::peek_next() const
	{
		if (is_at_end()) {
			return '\0';
		}

		return m_Current[1];
	}

	bool Lexer::match(char c)
	{
		if (is_at_end()) {
			return false;
		}

		if (*m_Current != c) {
			return false;
		}

		advance();
		return true;
	}

	bool Lexer::is_at_end() const
	{
		return *m_Current == '\0';
	}

	bool Lexer::is_digit(char c) const
	{
		return c >= '0' && c <= '9' || c == '_';
	}

	bool Lexer::is_alpha(char c) const
	{
		return c >= 'a' && c <= 'z'
			|| c >= 'A' && c <= 'Z'
			|| std::string("_&|").find(c) != std::string::npos;
	}

	bool Lexer::is_alnum(char c) const
	{
		return is_digit(c) || is_alpha(c);
	}

	Token Lexer::make_token(TokenType type)
	{
		Token token;
		token.type = type;
		token.start = m_Start;
		token.length = (uint32_t)(m_Current - m_Start);
		token.column = (uint32_t)(m_Start - m_LineStart);
		token.line = m_Line;
		return token;
	}

	Token Lexer::error_token(const char* err)
	{
		Token token;
		token.type = TokenType::Error;
		
		size_t err_size = strlen(err);
		char* error = (char*)malloc(err_size);
		if (error) {
			memcpy(error, err, err_size);
		}

		token.start = error;
		token.length = (uint32_t)err_size;
		token.column = (uint32_t)(m_Start - m_LineStart);
		token.line = m_Line;
		return token;
	}

}
