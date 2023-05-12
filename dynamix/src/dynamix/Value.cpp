#include  "Value.h"

#include "Object.h"

namespace dynamix {

	const char* value_type_to_string(ValueType value_type, ObjType* obj_type) {
		switch (value_type) {
			case ValueType::Number:    return "number";
			case ValueType::Bool:      return "bool";
			case ValueType::Character: return "char";
			case ValueType::Null:      return "null";
			case ValueType::Obj: {
				if (!obj_type) {
					__debugbreak();
				}

				switch (*obj_type) {
					case ObjType::Function: return "Function";
					case ObjType::String: return "String";
				}
			}
		}

		__debugbreak();
		return "None";
	}

    bool Value::is_object_type(ObjType type) const
    {
		return is_object() && as.object->type == type;
    }

	bool Value::is_function() const
	{
		return is_object_type(ObjType::Function);
	}

	bool Value::is_string() const
	{
		return is_object_type(ObjType::String);
	}

    ObjFunction* Value::as_function() const
	{
		if (!is_function()) {
			return nullptr;
		}

		return (ObjFunction*)as.object;
	}

	ObjString* Value::as_string() const
	{
		if (!is_string()) {
			return nullptr;
		}

		return (ObjString*)as.object;
	}

	void Value::print(bool new_line) const
	{
		auto func = [&]() { return (new_line ? "\n" : ""); };

		switch (type) {
			case ValueType::Number:    std::cout << as.number << func(); break;
			case ValueType::Bool:      std::cout << (as.boolean ? "true" : "false") << func(); break;
			case ValueType::Character: std::cout << as.character << func(); break;
			case ValueType::Null:      std::cout << "null" << func(); break;
			case ValueType::Obj: {
				switch (as.object->type) {
					case ObjType::Function:
						std::cout << std::format("<fn {}>", (as_function()->name.empty() ? "<script>" : as_function()->name.c_str())) << func();
						break;
					case ObjType::String:
						std::cout << as_string()->obj << func();
						break;
				}
			}
		}
	}
	
	bool Value::operator==(const Value& other) const
	{
		if (type != other.type) {
			return false;
		}

		if (is_object() && as.object->type != other.as.object->type) {
			return false;
		}

		switch (type)
		{
			case ValueType::Number:    return as.number == other.as.number;
			case ValueType::Bool:      return as.boolean == other.as.boolean;
			case ValueType::Character: return as.character == other.as.character;
			case ValueType::Null:      return true;
			case ValueType::Obj: {
				switch (as.object->type)
				{
					case ObjType::String: {
						std::string lhs = as_string()->obj;
						std::string rhs = other.as_string()->obj;
						if (lhs.size() != rhs.size()) {
							return false;
						}

						return memcmp(lhs.data(), rhs.data(), lhs.size()) == 0;
					}
					case ObjType::Function: {
						ObjFunction* lhs = as_function();
						ObjFunction* rhs = other.as_function();
						
						if (lhs->name.length() != rhs->name.length()) {
							return false;
						}

						if (lhs->name == rhs->name) {
							if (lhs->arity == rhs->arity) {
								return true;
							}
						}

						return false;
					}
				}
			}
		}

		// unreachable
		__debugbreak();
		return false;
	}
}
