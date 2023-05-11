#pragma once

#include "Value.h"

#include <vector>
#include <cstdint>

namespace dynamix {

	enum class OpCode : uint8_t
	{
		PushConstant,
		Pop,
		Null,
		True,
		False,
		Equal,
		Greater,
		Less,
		Add,
		Sub,
		Div,
		Mul,
		Negate,
		Not,
		Jmp,
		Jz,
		DefineGlobal,
		GetGlobal,
		SetGlobal,
		GetLocal,
		SetLocal,
		Print,
		Return,
	};

	struct ByteBlock
	{
	public:
		std::vector<uint8_t> bytes;
		std::vector<Value> constants;
		std::vector<uint32_t> lines;
		std::vector<std::string> source_lines;

		ByteBlock(const std::vector<std::string>& source_lines);

		void write_byte(uint8_t byte, uint32_t line);
		int32_t add_constant(Value value);
	};

}
