#include "Disassembler.h"

#include "dynamix.h"

#include <iostream>
#include <format>

namespace dynamix {

	void Disassembler::disassemble_block(ByteBlock* block, const char* name)
	{
		std::cout << std::format("-- {} --\n", name);

		for (uint32_t offset = 0; offset < block->bytes.size();) {
			offset = disassemble_instruction(block, offset);
		}
	}

	int32_t Disassembler::disassemble_instruction(ByteBlock* block, int32_t offset)
	{
		printf("%04d ", offset);

		if (offset > 0 && block->lines[offset] == block->lines[(size_t)offset - 1]) {
			printf("   | ");
		}
		else {
			printf("%4d ", block->lines[offset]);
		}

		uint8_t instruction = block->bytes[offset];
		switch ((OpCode)instruction) {
			case OpCode::Constant: return constant_instruction("OP_CONSTANT", block, offset);
			case OpCode::Negate: return simple_instruction("OP_NEGATE", offset);
			case OpCode::Not: return simple_instruction("OP_NOT", offset);
			case OpCode::Add: return simple_instruction("OP_ADD", offset);
			case OpCode::Sub: return simple_instruction("OP_SUB", offset);
			case OpCode::Mul: return simple_instruction("OP_MUL", offset);
			case OpCode::Div: return simple_instruction("OP_DIV", offset);
			case OpCode::Pop: return simple_instruction("OP_POP", offset);
			case OpCode::Return: return simple_instruction("OP_RETURN", offset);
			default:
				printf("Unknown opcode %d\n", instruction);
				return offset + 1;
		}
	}

	int32_t Disassembler::simple_instruction(const char* name, int32_t offset)
	{
		printf("%s\n", name);
		return offset + 1;
	}

	int32_t Disassembler::constant_instruction(const char* name, ByteBlock* block, int32_t offset)
	{
		uint8_t constant = block->bytes[(size_t)offset + 1];
		printf("%-16s %4d '", name, constant);
		print_value(block->constants[constant]);
		printf("'\n");
		return offset + 2;
	}

}
