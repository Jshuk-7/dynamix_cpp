#pragma once

#include "ByteBlock.h"
#include "Lexer.h"

#include <string>

namespace dynamix {

	class Compiler
	{
	public:
		Compiler(const std::string& source);

		void compile();

		ByteBlock byte_code() const;

	private:
		Lexer m_Lexer;
	};

}
