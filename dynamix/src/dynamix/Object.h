#pragma once

#include <string>

namespace dynamix {

	enum class ObjType
	{
		String,
	};

	struct Obj
	{
		ObjType type;
	};

	struct ObjString
	{
		std::string obj;
	};

}
