#pragma once

#include "ByteBlock.h"

#include <string>

namespace dynamix {

	enum class ObjType
	{
		Function,
		String,
	};

	struct Obj
	{
		ObjType type;
	};

	struct ObjFunction
	{
		uint32_t arity;
		ByteBlock block;
		std::string name;
	};

	struct ObjString
	{
		std::string obj;
	};

}
