#include "Compiler.h"

#include "dynamix.h"
#include "Object.h"
#include "Disassembler.h"

#include <format>

namespace dynamix {

#define LOCAL_CAPACITY 256

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
			{ TokenType::And,       ParseRule{ nullptr,            BIND_FN(and_),   Precedence::And } },
			{ TokenType::Struct,    ParseRule{ nullptr,            nullptr,         Precedence::None } },
			{ TokenType::Else,      ParseRule{ nullptr,            nullptr,         Precedence::None } },
			{ TokenType::False,     ParseRule{ BIND_FN(literal),   nullptr,         Precedence::None } },
			{ TokenType::For,       ParseRule{ nullptr,            nullptr,         Precedence::None } },
			{ TokenType::Fun,       ParseRule{ nullptr,            nullptr,         Precedence::None } },
			{ TokenType::If,        ParseRule{ nullptr,            nullptr,         Precedence::None } },
			{ TokenType::Null,      ParseRule{ BIND_FN(literal),   nullptr,         Precedence::None } },
			{ TokenType::Or,        ParseRule{ nullptr,            BIND_FN(or_),    Precedence::Or } },
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
	)
	{
		m_Function = nullptr;
		m_Type = FunctionType::Script;

		m_Locals.reserve(LOCAL_CAPACITY);
		m_ScopeDepth = 0;

		m_Function = new ObjFunction();
		m_Function->arity = 0;
		m_Function->block = ByteBlock();
		m_Function->name = "";
		((Obj*)m_Function)->type = ObjType::Function;

		Local local;
		local.depth = 0;
		local.name.start = "";
		local.name.length = 0;
		m_Locals.push(local);
	}

	ObjFunction* Compiler::compile()
	{
		advance();
		
		while (!match(TokenType::Eof)) {
			declaration();
		}

		consume(TokenType::Eof, "expected end of expression");
		push_return();

#if DEBUG_DISASSEMBLE_CODE
		if (!m_Parser.had_error) {
			ByteBlock block = current_byte_block();
			Disassembler::disassemble_block(&block, (m_Function->name.empty() ? "<script>" : m_Function->name.c_str()));
		}
#endif

		return m_Parser.had_error ? nullptr : m_Function;
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

	void Compiler::block()
	{
		while (!check(TokenType::RBracket) && !check(TokenType::Eof)) {
			declaration();
		}

		consume(TokenType::RBracket, "expected '}' after block");
	}

	void Compiler::statement()
	{
		if (match(TokenType::Print)) {
			print_statement();
		}
		else if (match(TokenType::LBracket)) {
			begin_scope();
			block();
			end_scope();
		}
		else if (match(TokenType::If)) {
			if_statement();
		}
		else if (match(TokenType::While)) {
			while_statement();
		}
		else if (match(TokenType::For)) {
			for_statement();
		}
		else {
			expression_statement();
		}
	}

	void Compiler::expression_statement()
	{
		expression();
		consume(TokenType::Semicolon, "expected ';' after expression");
		push_byte((uint8_t)OpCode::Pop);
	}

	void Compiler::print_statement()
	{
		expression();
		consume(TokenType::Semicolon, "expected ';' after expression");
		push_byte((uint8_t)OpCode::Print);
	}

	void Compiler::if_statement()
	{
		expression();

		int32_t then_jump = push_jump((uint8_t)OpCode::Jz);
		push_byte((uint8_t)OpCode::Pop);
		if (match(TokenType::LBracket)) {
			begin_scope();
			block();
			end_scope();
		}
		else {
			statement();
		}

		int32_t else_jump = push_jump((uint8_t)OpCode::Jmp);

		patch_jump(then_jump);
		push_byte((uint8_t)OpCode::Pop);

		if (match(TokenType::Else)) {
			if (match(TokenType::LBracket)) {
				begin_scope();
				block();
				end_scope();
			}
			else {
				statement();
			}
		}

		patch_jump(else_jump);
	}

	void Compiler::while_statement()
	{
		int32_t loop_start = current_byte_block().bytes.size();
		expression();

		int32_t exit_jump = push_jump((uint8_t)OpCode::Jz);
		push_byte((uint8_t)OpCode::Pop);
		if (match(TokenType::LBracket)) {
			begin_scope();
			block();
			end_scope();
		}
		else {
			statement();
		}

		push_loop(loop_start);

		patch_jump(exit_jump);
		push_byte((uint8_t)OpCode::Pop);
	}

	void Compiler::for_statement()
	{
		begin_scope();
		consume(TokenType::LParen, "expected '(' after for");
		if (match(TokenType::Semicolon)) {
			// nothing
		}
		else if (match(TokenType::Let)) {
			let_declaration();
		}
		else {
			expression_statement();
		}

		int32_t loop_start = current_byte_block().bytes.size();
		int32_t exit_jump = -1;
		if (!match(TokenType::Semicolon)) {
			expression();
			consume(TokenType::Semicolon, "expected ';' after loop condition");

			exit_jump = push_jump((uint8_t)OpCode::Jz);
			push_byte((uint8_t)OpCode::Pop);
		}

		if (!match(TokenType::RParen)) {
			int32_t body_jump = push_jump((uint8_t)OpCode::Jmp);
			int32_t increment_start = current_byte_block().bytes.size();
			expression();
			push_byte((uint8_t)OpCode::Pop);
			consume(TokenType::RParen, "expected ')'");

			push_loop(loop_start);
			loop_start = increment_start;
			patch_jump(body_jump);
		}

		if (match(TokenType::LBracket)) {
			block();
		}
		else {
			statement();
		}

		push_loop(loop_start);
		
		if (exit_jump != -1) {
			patch_jump(exit_jump);
			push_byte((uint8_t)OpCode::Pop);
		}

		end_scope();
	}

	void Compiler::declaration()
	{
		if (match(TokenType::Let)) {
			let_declaration();
		}
		else {
			statement();
		}

		if (m_Parser.panic_mode) {
			synchronize();
		}
	}

	void Compiler::let_declaration()
	{
		uint8_t global = parse_variable("expected identifier");

		if (match(TokenType::Eq)) {
			expression();
		}
		else {
			push_byte((uint8_t)OpCode::Null);
		}

		consume(TokenType::Semicolon, "expected ';' after expression");

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

	void Compiler::push_loop(int32_t loop_start)
	{
		push_byte((uint8_t)OpCode::Loop);

		int32_t offset = current_byte_block().bytes.size() - loop_start + 2;
		if (offset > UINT16_MAX) {
			error("Loop body too large");
		}

		push_byte((offset >> 8) & 0xff);
		push_byte(offset & 0xff);
	}

	int32_t Compiler::push_jump(uint8_t instruction)
	{
		push_byte(instruction);
		push_byte(0xff);
		push_byte(0xff);
		return (int32_t)current_byte_block().bytes.size() - 2;
	}

	void Compiler::push_constant(Value value)
	{
		push_bytes((uint8_t)OpCode::PushConstant, make_constant(value));
	}

	void Compiler::push_return()
	{
		push_byte((uint8_t)OpCode::Return);
	}

	void Compiler::patch_jump(int32_t offset)
	{
		int32_t jump = (int32_t)current_byte_block().bytes.size() - offset - 2;

		if (jump > UINT16_MAX) {
			error("Too much code to jump over");
		}

		current_byte_block().bytes[offset] = (jump >> 8) & 0xff;
		current_byte_block().bytes[offset + 1] = jump & 0xff;
	}

	void Compiler::binary(bool can_assign)
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

	void Compiler::literal(bool can_assign)
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

	void Compiler::grouping(bool can_assign)
	{
		expression();
		consume(TokenType::RParen, "expected ')' after expression");
	}

	void Compiler::named_variable(const Token* name, bool can_assign)
	{
		uint8_t get_op;
		uint8_t set_op;

		int32_t constant = resolve_local(name);
		if (constant != -1) {
			get_op = (uint8_t)OpCode::GetLocal;
			set_op = (uint8_t)OpCode::SetLocal;
		}
		else {
			constant = (int32_t)identifier_constant(name);
			get_op = (uint8_t)OpCode::GetGlobal;
			set_op = (uint8_t)OpCode::SetGlobal;
		}

		if (can_assign && match(TokenType::Eq)) {
			expression();
			push_bytes(set_op, (uint8_t)constant);
		}
		else {
			push_bytes(get_op, (uint8_t)constant);
		}
	}

	void Compiler::variable(bool can_assign)
	{
		named_variable(&m_Parser.previous, can_assign);
	}

	void Compiler::string(bool can_assign)
	{
		std::string string(m_Parser.previous.start + 1, m_Parser.previous.length - 2);
		string.push_back('\0');
		
		ObjString* object = new ObjString();
		object->obj.resize(string.size());
		object->obj.assign(string);
		((Obj*)object)->type = ObjType::String;

		push_constant(Value((Obj*)object));
	}

	void Compiler::number(bool can_assign)
	{
		std::string number_string(m_Parser.previous.start, m_Parser.previous.length);
		remove_char_from_string(number_string, '_');
		remove_char_from_string(number_string, '\'');
		number_string.push_back('\0');

		double number = std::strtod(number_string.c_str(), nullptr);
		push_constant(Value(number));
	}

	void Compiler::character(bool can_assign)
	{
		char character = m_Parser.previous.start[0];
		push_constant(Value(character));
	}

	void Compiler::unary(bool can_assign)
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

	void Compiler::and_(bool can_assign)
	{
		int32_t end_jump = push_jump((uint8_t)OpCode::Jz);
		push_byte((uint8_t)OpCode::Pop);

		parse_precedence(Precedence::And);

		patch_jump(end_jump);
	}

	void Compiler::or_(bool can_assign)
	{
		int32_t else_jump = push_jump((uint8_t)OpCode::Jz);
		int32_t end_jump = push_jump((uint8_t)OpCode::Jmp);

		patch_jump(else_jump);
		push_byte((uint8_t)OpCode::Pop);

		parse_precedence(Precedence::Or);
		patch_jump(end_jump);
	}

	void Compiler::begin_scope()
	{
		m_ScopeDepth++;
	}

	void Compiler::end_scope()
	{
		m_ScopeDepth--;

		while (!m_Locals.is_empty() && m_Locals[m_Locals.size() - 1].depth > m_ScopeDepth) {
			push_byte((uint8_t)OpCode::Pop);
			m_Locals.pop();
		}
	}

	void Compiler::parse_precedence(Precedence precedence)
	{
		advance();
		ParseFn prefix_rule = m_ParseRules.at(m_Parser.previous.type).prefix;
		if (!prefix_rule) {
			error("expected expression");
			return;
		}

		bool can_assign = precedence <= Precedence::Assign;
		prefix_rule(can_assign);

		while (precedence <= m_ParseRules.at(m_Parser.current.type).precedence) {
			advance();
			ParseFn infix_rule = m_ParseRules.at(m_Parser.previous.type).infix;
			infix_rule(can_assign);
		}

		if (can_assign && match(TokenType::Eq)) {
			error("invalid assignment target");
		}
	}

	uint8_t Compiler::identifier_constant(const Token* name)
	{
		ObjString* object = new ObjString();
		((Obj*)object)->type = ObjType::String;

		std::string identifier(name->start, name->length);

		object->obj = identifier;

		return make_constant(Value((Obj*)object));
	}

	bool Compiler::identifiers_equal(const Token* name, const Token* other) const
	{
		if (name->length != other->length) {
			return false;
		}

		return memcmp(name->start, other->start, name->length) == 0;
	}

	int32_t Compiler::resolve_local(const Token* name)
	{
		for (int32_t i = m_Locals.size() - 1; i >= 0; i--) {
			const Local* local = &m_Locals[(size_t)i];
			if (identifiers_equal(name, &local->name)) {
				if (local->depth == -1) {
					std::string var_name(local->name.start, local->name.length);
					var_name.push_back('\0');

					error(std::format("uninitialized local variable '{}' used", var_name));
				}

				return (int32_t)i;
			}
		}

		return -1;
	}

	void Compiler::add_local(const Token* name)
	{
		if (m_Locals.size() == LOCAL_CAPACITY) {
			error("too many local variables in function");
			return;
		}

		Local local;
		local.name = *name;
		local.depth = -1;
		m_Locals.push(local);
	}

	void Compiler::declare_variable()
	{
		if (m_ScopeDepth == 0) {
			return;
		}

		Token* name = &m_Parser.previous;
		for (int32_t i = m_Locals.size() - 1; i >= 0; i--) {
			Local* local = &m_Locals[(size_t)i];
			if (local->depth != -1 && local->depth < (int32_t)m_ScopeDepth) {
				break;
			}

			if (identifiers_equal(name, &local->name)) {
				std::string var_name(name->start, name->length);
				var_name.push_back('\0');

				error(std::format("variable '{}' has multiple definitions; multiple initialization", var_name));
			}
		}

		add_local(name);
	}

	uint8_t Compiler::parse_variable(const std::string& error)
	{
		consume(TokenType::Ident, error);

		declare_variable();
		if (m_ScopeDepth > 0) {
			return 0;
		}

		return identifier_constant(&m_Parser.previous);
	}

	void Compiler::mark_initialized()
	{
		m_Locals[m_Locals.size() - 1].depth = m_ScopeDepth;
	}

	void Compiler::define_variable(uint8_t global)
	{
		if (m_ScopeDepth > 0) {
			mark_initialized();
			return;
		}

		push_bytes((uint8_t)OpCode::DefineGlobal, global);
	}

	uint8_t Compiler::make_constant(Value value)
	{
		int32_t constant = current_byte_block().add_constant(value);
		if (constant > UINT8_MAX) {
			error("too many constants in one block");
			return 0;
		}

		return (uint8_t)constant;
	}

	void Compiler::remove_char_from_string(std::string& str, char c) const
	{
		while (size_t pos = str.find(c)) {
			if (pos == std::string::npos) {
				return;
			}

			str.replace(pos, 1, "");
		}
	}

	ByteBlock& Compiler::current_byte_block()
	{
		return m_Function->block;
	}

	void Compiler::error(const std::string& msg)
	{
		error_at(&m_Parser.previous, msg);
	}

	void Compiler::error_at(const Token* token, const std::string& msg)
	{
		if (m_Parser.panic_mode) {
			return;
		}

		m_Parser.panic_mode = true;
		std::string error;
		error += std::format("<{}:{}:{}> Compiler Error", m_Filename, token->line, token->column);

		if (token->type == TokenType::Eof) {
			error += " at end";
		}
		else if (token->type == TokenType::Error) {
			// Nothing
		}
		else {
			const char* format = " at '%.*s'";
			size_t buf_size = token->length + strlen(format);
			char* buf = (char*)malloc(buf_size);
			if (buf) {
				sprintf_s(buf, buf_size, format, token->length, token->start);
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
		error_at(&m_Parser.current, msg);
	}

}
