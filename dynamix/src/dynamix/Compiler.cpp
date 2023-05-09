#include "Compiler.h"

namespace dynamix {

	Compiler::Compiler(const std::string& source)
		: m_Lexer(source) { }

	void Compiler::compile()
	{
		uint32_t line = -1;
		for (;;) {
			Token token = m_Lexer.scan_token();
			if (token.line != line) {
				printf("%4d ", token.line);
				line = token.line;
			}
			else {
				printf("   | ");
			}

			printf("%2d '%.*s'\n", token.type, token.length, token.start);

			if (token.type == TokenType::Eof) {
				break;
			}
		}
	}

	ByteBlock Compiler::byte_code() const
	{
		return ByteBlock();
	}

}
