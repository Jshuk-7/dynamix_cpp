#include "Compiler.h"

#include "Disassembler.h"

#include <format>

namespace dynamix {

	using namespace std::placeholders;
#define BIND_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

	Compiler::Compiler(const std::string& filename, const std::string& source)
		: m_Filename(filename), m_Lexer(source), m_Parser(), m_ParseRules(
		{
			{ TokenType::LParen,    ParseRule{ BIND_FN(grouping),  nullptr,         Precedence::None } },
			{ TokenType::RParen,    ParseRule{ nullptr,            nullptr,         Precedence::None } },
			{ TokenType::LBracket,  ParseRule{ nullptr,            nullptr,         Precedence::None } },
			{ TokenType::RBracket,  ParseRule{ nullptr,            nullptr,         Precedence::None } },
			{ TokenType::Comma,     ParseRule{ nullptr,            nullptr,         Precedence::None } },
			{ TokenType::Dot,       ParseRule{ nullptr,            nullptr,         Precedence::None } },
			{ TokenType::Minus,     ParseRule{ BIND_FN(unary),     BIND_FN(binary), Precedence::Term } },
			{ TokenType::Plus,      ParseRule{ nullptr,            BIND_FN(binary), Precedence::Term } },
			{ TokenType::Semicolon, ParseRule{ nullptr,            nullptr,         Precedence::None } },
			{ TokenType::Slash,     ParseRule{ nullptr,            BIND_FN(binary), Precedence::Factor } },
			{ TokenType::Star,      ParseRule{ nullptr,            BIND_FN(binary), Precedence::Factor } },
			{ TokenType::Bang,      ParseRule{ BIND_FN(unary),     nullptr,         Precedence::None } },
			{ TokenType::BangEq,    ParseRule{ nullptr,            BIND_FN(binary), Precedence::Equality } },
			{ TokenType::Eq,        ParseRule{ nullptr,            nullptr,         Precedence::None } },
			{ TokenType::EqEq,      ParseRule{ nullptr,            BIND_FN(binary), Precedence::Equality } },
			{ TokenType::Gt,        ParseRule{ nullptr,            BIND_FN(binary), Precedence::Comparison } },
			{ TokenType::Gte,       ParseRule{ nullptr,            BIND_FN(binary), Precedence::Comparison } },
			{ TokenType::Lt,        ParseRule{ nullptr,            BIND_FN(binary), Precedence::Comparison } },
			{ TokenType::Lte,       ParseRule{ nullptr,            BIND_FN(binary), Precedence::Comparison } },
			{ TokenType::Ident,     ParseRule{ BIND_FN(variable),  nullptr,         Precedence::None}},
			{ TokenType::String,    ParseRule{ BIND_FN(string),    nullptr,         Precedence::None}},
			{ TokenType::Number,    ParseRule{ BIND_FN(number),    nullptr,         Precedence::None } },
			{ TokenType::Char,      ParseRule{ BIND_FN(character), nullptr,         Precedence::None } },
			{ TokenType::And,       ParseRule{ nullptr,            nullptr,         Precedence::None } },
			{ TokenType::Struct,    ParseRule{ nullptr,            nullptr,         Precedence::None } },
			{ TokenType::Else,      ParseRule{ nullptr,            nullptr,         Precedence::None } },
			{ TokenType::False,     ParseRule{ BIND_FN(literal),   nullptr,         Precedence::None } },
			{ TokenType::For,       ParseRule{ nullptr,            nullptr,         Precedence::None } },
			{ TokenType::Fun,       ParseRule{ nullptr,            nullptr,         Precedence::None } },
			{ TokenType::If,        ParseRule{ nullptr,            nullptr,         Precedence::None } },
			{ TokenType::Null,      ParseRule{ BIND_FN(literal),   nullptr,         Precedence::None } },
			{ TokenType::Or,        ParseRule{ nullptr,            nullptr,         Precedence::None } },
			{ TokenType::Print,     ParseRule{ nullptr,            nullptr,         Precedence::None } },
			{ TokenType::Return,    ParseRule{ nullptr,            nullptr,         Precedence::None } },
			{ TokenType::Super,     ParseRule{ nullptr,            nullptr,         Precedence::None } },
			{ TokenType::Self,      ParseRule{ nullptr,            nullptr,         Precedence::None } },
			{ TokenType::True,      ParseRule{ BIND_FN(literal),   nullptr,         Precedence::None } },
			{ TokenType::Let,       ParseRule{ nullptr,            nullptr,         Precedence::None } },
			{ TokenType::While,     ParseRule{ nullptr,            nullptr,         Precedence::None } },
			{ TokenType::Error,     ParseRule{ nullptr,            nullptr,         Precedence::None } },
			{ TokenType::Eof,       ParseRule{ nullptr,            nullptr,         Precedence::None } },
		}
	) { }

	bool Compiler::compile(ByteBlock* byte_code)
	{
		m_ByteBlock = byte_code;
		advance();
		
		while (!match(TokenType::Eof)) {
			declaration();
		}

		consume(TokenType::Eof, "Expected end of expression");
		push_return();

#if DEBUG_PRINT_CODE
		if (!m_Parser.had_error) {
			ByteBlock block = current_byte_block();
			Disassembler::disassemble_block(&block, "code");
		}
#endif

		return !m_Parser.had_error;
	}

	const std::string& Compiler::get_last_error() const
	{
		return m_LastError;
	}

	Token Compiler::advance()
	{
		m_Parser.previous = m_Parser.current;

		for (;;) {
			m_Parser.current = m_Lexer.scan_token();

			if (m_Parser.current.type != TokenType::Error)
				break;

			std::string err(m_Parser.current.start, (size_t)m_Parser.current.length);
			err.push_back('\0');
			error_at_current(err);

			// we can drop the error now
			free((char*)m_Parser.current.start);
		}

		return m_Parser.current;
	}

	void Compiler::expression()
	{
		parse_precedence(Precedence::Assign);
	}

	void Compiler::expression_statement()
	{
		expression();
		consume(TokenType::Semicolon, "Expected ';' after expression");
		push_byte((uint8_t)OpCode::Pop);
	}

	void Compiler::print_statement()
	{
		expression();
		consume(TokenType::Semicolon, "Expected ';' after expression");
		push_byte((uint8_t)OpCode::Print);
	}

	void Compiler::declaration()
	{
		if (match(TokenType::Let)) {
			let_declaration();
		}
		else if (match(TokenType::Print)) {
			print_statement();
		}
		else {
			expression_statement();
		}

		if (m_Parser.panic_mode) {
			synchronize();
		}
	}

	void Compiler::let_declaration()
	{
		uint8_t global = parse_variable("Expected identifier");

		if (match(TokenType::Eq)) {
			expression();
		}
		else {
			push_byte((uint8_t)OpCode::Null);
		}

		consume(TokenType::Semicolon, "Expected ';' after expression");

		define_variable(global);
	}

	void Compiler::synchronize()
	{
		m_Parser.panic_mode = false;

		while (!check(TokenType::Eof)) {
			if (m_Parser.previous.type == TokenType::Semicolon) {
				return;
			}

			switch (m_Parser.current.type) {
				case TokenType::Struct:
				case TokenType::Fun:
				case TokenType::Let:
				case TokenType::For:
				case TokenType::If:
				case TokenType::While:
				case TokenType::Print:
				case TokenType::Return:
					return;
				default:
					;
			}

			advance();
		}
	}

	void Compiler::consume(TokenType expected, const std::string& msg)
	{
		if (!check(expected)) {
			error_at_current(msg);
			return;
		}

		advance();
	}

	bool Compiler::match(TokenType type)
	{
		if (!check(type)) {
			return false;
		}

		advance();
		return true;
	}

	bool Compiler::check(TokenType type)
	{
		return m_Parser.current.type == type;
	}

	void Compiler::push_byte(uint8_t byte)
	{
		current_byte_block().write_byte(byte, m_Parser.previous.line);
	}

	void Compiler::push_bytes(uint8_t one, uint8_t two)
	{
		push_byte(one);
		push_byte(two);
	}

	void Compiler::push_constant(Value value)
	{
		push_bytes((uint8_t)OpCode::PushConstant, make_constant(value));
	}

	void Compiler::push_return()
	{
		push_byte((uint8_t)OpCode::Return);
	}

	void Compiler::binary()
	{
		TokenType operator_type = m_Parser.previous.type;
		ParseRule rule = m_ParseRules.at(operator_type);
		parse_precedence((Precedence)((uint32_t)rule.precedence + 1));

		switch (operator_type) {
			case TokenType::BangEq: push_bytes((uint8_t)OpCode::Equal, (uint8_t)OpCode::Not);   break;
			case TokenType::EqEq:   push_byte((uint8_t)OpCode::Equal);                          break;
			case TokenType::Gt:     push_byte((uint8_t)OpCode::Greater);                        break;
			case TokenType::Gte:    push_bytes((uint8_t)OpCode::Less, (uint8_t)OpCode::Not);    break;
			case TokenType::Lt:     push_byte((uint8_t)OpCode::Less);                           break;
			case TokenType::Lte:    push_bytes((uint8_t)OpCode::Greater, (uint8_t)OpCode::Not); break;
			case TokenType::Plus:   push_byte((uint8_t)OpCode::Add);                            break;
			case TokenType::Minus:  push_byte((uint8_t)OpCode::Sub);                            break;
			case TokenType::Star:   push_byte((uint8_t)OpCode::Mul);                            break;
			case TokenType::Slash:  push_byte((uint8_t)OpCode::Div);                            break;
			default:
				// unreachable
				return;
		}
	}

	void Compiler::literal()
	{
		switch (m_Parser.previous.type) {
			case TokenType::Null:  push_byte((uint8_t)OpCode::Null);  break;
			case TokenType::True:  push_byte((uint8_t)OpCode::True);  break;
			case TokenType::False: push_byte((uint8_t)OpCode::False); break;
			default:
				// unreachable
				return;
		}
	}

	void Compiler::grouping()
	{
		expression();
		consume(TokenType::RParen, "Expected ')' after expression");
	}

	void Compiler::named_variable(Token* name)
	{
		uint8_t constant = identifier_constant(name);
		push_bytes((uint8_t)OpCode::GetGlobal, constant);
	}

	void Compiler::variable()
	{
		named_variable(&m_Parser.previous);
	}

	void Compiler::string()
	{
		std::string string(m_Parser.previous.start + 1, m_Parser.previous.length - 2);
		string.push_back('\0');
		
		ObjString* object = new ObjString();
		((Obj*)object)->type = ObjType::String;
		object->obj = string;

		push_constant(Value((Obj*)object));
	}

	void Compiler::number()
	{
		double number = strtod(m_Parser.previous.start, nullptr);
		push_constant(Value(number));
	}

	void Compiler::character()
	{
		char character = m_Parser.previous.start[0];
		push_constant(Value(character));
	}

	void Compiler::unary()
	{
		TokenType operator_type = m_Parser.previous.type;

		parse_precedence(Precedence::Unary);

		switch (operator_type) {
			case TokenType::Minus: push_byte((uint8_t)OpCode::Negate); break;
			case TokenType::Bang:  push_byte((uint8_t)OpCode::Not);    break;
			default:
				// unreachable
				return;
		}
	}

	void Compiler::parse_precedence(Precedence precedence)
	{
		advance();
		ParseFn prefix_rule = m_ParseRules.at(m_Parser.previous.type).prefix;
		if (!prefix_rule) {
			error("Expected expression");
			return;
		}

		prefix_rule();

		while (precedence <= m_ParseRules.at(m_Parser.current.type).precedence) {
			advance();
			ParseFn infix_rule = m_ParseRules.at(m_Parser.previous.type).infix;
			infix_rule();
		}
	}

	uint8_t Compiler::identifier_constant(Token* name)
	{
		ObjString* object = new ObjString();
		((Obj*)object)->type = ObjType::String;

		std::string identifier(name->start, name->length);

		object->obj = identifier;

		return make_constant(Value((Obj*)object));
	}

	uint8_t Compiler::parse_variable(const std::string& error)
	{
		consume(TokenType::Ident, error);
		return identifier_constant(&m_Parser.previous);
	}

	void Compiler::define_variable(uint8_t global)
	{
		push_bytes((uint8_t)OpCode::DefineGlobal, global);
	}

	uint8_t Compiler::make_constant(Value value)
	{
		int32_t constant = current_byte_block().add_constant(value);
		if (constant > UINT8_MAX) {
			error("Too many constants in one block");
			return 0;
		}

		return (uint8_t)constant;
	}

	ByteBlock& Compiler::current_byte_block()
	{
		return *m_ByteBlock;
	}

	void Compiler::error(const std::string& msg)
	{
		error_at(m_Parser.previous, msg);
	}

	void Compiler::error_at(Token token, const std::string& msg)
	{
		if (m_Parser.panic_mode) {
			return;
		}

		m_Parser.panic_mode = true;
		std::string error;
		error += std::format("<{}:{}:{}> Compiler Error", m_Filename, token.column, token.line);

		if (token.type == TokenType::Eof) {
			error += " at end";
		}
		else if (token.type == TokenType::Error) {
			// Nothing
		}
		else {
			const char* format = " at '%.*s'";
			size_t buf_size = token.length + strlen(format);
			char* buf = (char*)malloc(buf_size);
			if (buf) {
				sprintf_s(buf, buf_size, format, token.length, token.start);
				error.append(buf);
			}
			free(buf);
		}

		error += std::format(": {}\n", msg);
		m_LastError = error;
		m_Parser.had_error = true;
	}

	void Compiler::error_at_current(const std::string& msg)
	{
		error_at(m_Parser.current, msg);
	}

}
