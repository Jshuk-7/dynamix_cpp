#pragma once

#include "ByteBlock.h"
#include "Disassembler.h"

#include <iostream>

namespace dynamix {

	static void runtime_start(int argc, char* argv[])
	{
		ByteBlock block;

		int32_t constant = block.add_constant(1.2);
		block.write_byte((uint8_t)OpCode::Constant, 123);
		block.write_byte((uint8_t)constant, 123);
		block.write_byte((uint8_t)OpCode::Return, 124);

		Disassembler::disassemble_block(&block, "test block");

		std::cin.get();
	}

	static void print_value(Value value)
	{
		printf("%g", value);
	}

}