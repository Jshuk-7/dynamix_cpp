#pragma once

#include "Value.h"

#include <vector>
#include <cstdint>

namespace dynamix {

	enum class OpCode : uint8_t
	{
		Constant,
		Pop,
		Return,
	};

	class ByteBlock
	{
	public:
		std::vector<uint8_t> bytes;
		std::vector<Value> constants;
		std::vector<uint32_t> lines;

		void write_byte(uint8_t byte, uint32_t line);
		int32_t add_constant(Value value);
	};

}
