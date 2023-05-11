#pragma once

#include "ByteBlock.h"
#include "Lexer.h"
#include "Value.h"
#include "Stack.h"

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

	using ParseFn = std::function<void(bool)>;

	struct ParseRule
	{
		ParseFn prefix;
		ParseFn infix;
		Precedence precedence;
	};

	struct Local
	{
		Token name;
		int32_t depth;
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
		void block();
		void statement();
		void expression_statement();
		void print_statement();
		void if_statement();
		void while_statement();
		void for_statement();
		void declaration();
		void let_declaration();

		void synchronize();
		
		void consume(TokenType expected, const std::string& msg);
		bool match(TokenType type);
		bool check(TokenType type);

		void push_byte(uint8_t byte);
		void push_bytes(uint8_t one, uint8_t two);
		int32_t push_jump(uint8_t instruction);
		void push_constant(Value value);
		void push_return();

		void patch_jump(int32_t offset);
		
		void binary(bool can_assign);
		void literal(bool can_assign);
		void grouping(bool can_assign);
		void named_variable(const Token* name, bool can_assign);
		void variable(bool can_assign);
		void string(bool can_assign);
		void number(bool can_assign);
		void character(bool can_assign);
		void unary(bool can_assign);
		
		void begin_scope();
		void end_scope();

		void parse_precedence(Precedence precedence);
		uint8_t identifier_constant(const Token* name);
		bool identifiers_equal(const Token* name, const Token* other) const;
		int32_t resolve_local(const Token* name);
		void add_local(const Token* name);
		void declare_variable();
		uint8_t parse_variable(const std::string& error);
		void mark_initialized();
		void define_variable(uint8_t global);

		uint8_t make_constant(Value value);

		ByteBlock& current_byte_block();
		void error(const std::string& msg);
		void error_at(const Token* token, const std::string& msg);
		void error_at_current(const std::string& msg);

	private:
		std::string m_Filename;
		std::string m_LastError;
		
		ByteBlock* m_ByteBlock = nullptr;
		Lexer m_Lexer;
		Parser m_Parser;

		std::unordered_map<TokenType, ParseRule> m_ParseRules;

		Stack<Local> m_Locals;
		uint32_t m_ScopeDepth;
	};

}
