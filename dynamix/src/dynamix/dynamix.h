#pragma once

#include "ByteBlock.h"
#include "Disassembler.h"
#include "VirtualMachine.h"

#include <iostream>

namespace dynamix {

#define DEBUG_TRACE_EXECUTION 1

	static void runtime_start(int argc, char* argv[])
	{
		ByteBlock block;

		int32_t constant = block.add_constant(1.2);
		block.write_byte((uint8_t)OpCode::Constant, 123);
		block.write_byte((uint8_t)constant, 123);

		int32_t constant2 = block.add_constant(9.6);
		block.write_byte((uint8_t)OpCode::Constant, 124);
		block.write_byte((uint8_t)constant2, 124);

		block.write_byte((uint8_t)OpCode::Add, 124);

		block.write_byte((uint8_t)OpCode::Return, 126);

		VirtualMachine vm;
		vm.interpret(&block);

		std::cin.get();
	}

	static void print_value(Value value)
	{
		printf("%g", value);
	}

}