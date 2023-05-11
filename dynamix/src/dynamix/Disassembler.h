#pragma once

#include "ByteBlock.h"

namespace dynamix {

	class Disassembler
	{
	public:
		static void disassemble_block(ByteBlock* block, const char* name);
		static int32_t disassemble_instruction(ByteBlock* block, int32_t offset);

		static int32_t simple_instruction(const char* name, int32_t offset);
		static int32_t constant_instruction(const char* name, ByteBlock* block, int32_t offset);
		static int32_t byte_instruction(const char* name, ByteBlock* block, int32_t offset);
	};

}
