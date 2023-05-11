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
		void expression_statement();
		void print_statement();
		void declaration();
		void let_declaration();

		void synchronize();
		
		void consume(TokenType expected, const std::string& msg);
		bool match(TokenType type);
		bool check(TokenType type);

		void push_byte(uint8_t byte);
		void push_bytes(uint8_t one, uint8_t two);
		void push_constant(Value value);
		void push_return();
		
		void binary();
		void literal();
		void grouping();
		void named_variable(Token* name);
		void variable();
		void string();
		void number();
		void character();
		void unary();
		
		void parse_precedence(Precedence precedence);
		uint8_t identifier_constant(Token* name);
		uint8_t parse_variable(const std::string& error);
		void define_variable(uint8_t global);

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
