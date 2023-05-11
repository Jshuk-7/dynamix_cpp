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
			case OpCode::PushConstant: return constant_instruction("PUSH CONSTANT", block, offset);
			case OpCode::Pop:          return simple_instruction("POP", offset);
			case OpCode::Null:         return simple_instruction("NULL", offset);
			case OpCode::True:         return simple_instruction("TRUE", offset);
			case OpCode::False:        return simple_instruction("FALSE", offset);
			case OpCode::Equal:        return simple_instruction("EQUAL", offset);
			case OpCode::Greater:      return simple_instruction("GREATER", offset);
			case OpCode::Less:         return simple_instruction("LESS", offset);
			case OpCode::Add:          return simple_instruction("ADD", offset);
			case OpCode::Sub:          return simple_instruction("SUB", offset);
			case OpCode::Mul:          return simple_instruction("MUL", offset);
			case OpCode::Div:          return simple_instruction("DIV", offset);
			case OpCode::Negate:       return simple_instruction("NEGATE", offset);
			case OpCode::Not:          return simple_instruction("NOT", offset);
			case OpCode::DefineGlobal: return constant_instruction("DEFINE GLOBAL", block, offset);
			case OpCode::GetGlobal:    return constant_instruction("GET GLOBAL", block, offset);
			case OpCode::SetGlobal:    return constant_instruction("SET GLOBAL", block, offset);
			case OpCode::Print:        return simple_instruction("PRINT", offset);
			case OpCode::Return:       return simple_instruction("RETURN", offset);
			default:
				printf("Unknown opcode %d\n", instruction);
				return offset + 1;
		}
	}

	int32_t Disassembler::simple_instruction(const char* name, int32_t offset)
	{
		printf("OPCODE: %s\n", name);
		return offset + 1;
	}

	int32_t Disassembler::constant_instruction(const char* name, ByteBlock* block, int32_t offset)
	{
		uint8_t constant = block->bytes[(size_t)offset + 1];
		printf("OPCODE: %-16s %4d '", name, constant);
		block->constants[constant].print(false);
		printf("'\n");
		return offset + 2;
	}

}
