#include "ByteBlock.h"

#include <cstdlib>

namespace dynamix {

	void ByteBlock::write_byte(uint8_t byte, uint32_t line)
	{
		bytes.push_back(byte);
		lines.push_back(line);
	}

	int32_t ByteBlock::add_constant(Value value)
	{
		constants.push_back(value);
		return constants.size() - 1;
	}

}
