#pragma once

#include "ByteBlock.h"
#include "Lexer.h"
#include "Value.h"

#include <unordered_map>
#include <functional>
#include <string>

namespace dynamix {

	enum class Precedence
	{
		None,
		Assign,      // =
		Or,          // or
		And,         // and
		Equality,    // == !=
		Comparison,  // < > <= >=
		Term,        // + -
		Factor,      // * /
		Unary,       // ! -
		Call,        // . ()
		Atom
	};

	using ParseFn = std::function<void()>;

	struct ParseRule
	{
		ParseFn prefix;
		ParseFn infix;
		Precedence precedence;
	};

	struct Parser
	{
		Token previous;
		Token current;
		bool had_error = false;
		bool panic_mode = false;
	};

	class Compiler
	{
	public:
		Compiler(const std::string& filepath, const std::string& source);

		bool compile(ByteBlock* byte_code);

		const std::string& get_last_error() const;

	private:
		Token advance();
		void expression();
		void consume(TokenType expected, const std::string& msg);
		
		void emit_byte(uint8_t byte);
		void emit_bytes(uint8_t one, uint8_t two);
		void emit_constant(Value value);
		void emit_return();
		
		void binary();
		void literal();
		void grouping();
		void string();
		void number();
		void character();
		void unary();
		void parse_precedence(Precedence precedence);
		
		uint8_t make_constant(Value value);

		ByteBlock& current_byte_block();
		void error(const std::string& msg);
		void error_at(Token token, const std::string& msg);
		void error_at_current(const std::string& msg);

	private:
		std::unordered_map<TokenType, ParseRule> m_ParseRules;
		std::string m_Filename;
		std::string m_LastError;
		ByteBlock* m_ByteBlock = nullptr;
		Lexer m_Lexer;
		Parser m_Parser;
	};

}
