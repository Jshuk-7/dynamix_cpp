#pragma once

#include "Object.h"

#include <iostream>

namespace dynamix {

	enum class ValueType
	{
		Number,
		Bool,
		Character,
		Null,
		Obj,
	};

	static const char* value_type_to_string(ValueType value_type, ObjType* obj_type = nullptr) {
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
					case ObjType::String: return "String";
				}
			}
		}

		__debugbreak();
		return "None";
	}

	typedef struct Obj Obj;
	typedef struct ObjString ObjString;

	struct Value
	{
		ValueType type;
		union
		{
			double number;
			bool boolean;
			char character;
			Obj* object;
		} as;

		Value() {
			type = ValueType::Null;
			as.number = 0.0;
		}

		Value(double number) {
			type = ValueType::Number;
			as.number = number;
		}

		Value(bool boolean) {
			type = ValueType::Bool;
			as.boolean = boolean;
		}

		Value(char character) {
			type = ValueType::Character;
			as.character = character;
		}

		Value(std::nullptr_t) {
			type = ValueType::Null;
			as.number = 0.0;
		}

		Value(Obj* object) {
			type = ValueType::Obj;
			as.object = object;
		}

		bool is(ValueType _type) const {
			return type == _type;
		}

		bool is_object() const {
			return is(ValueType::Obj);
		}

		bool is_object_type(ObjType type) const {
			return is_object() && as.object->type == type;
		}

		bool is_string() const {
			return is_object_type(ObjType::String);
		}

		ObjString* as_string() const {
			if (!is_string()) {
				return nullptr;
			}

			return ((ObjString*)as.object);
		}

		void print(bool new_line) const {
			auto func = [&]() { return (new_line ? "\n" : ""); };

			switch (type) {
				case ValueType::Number:    std::cout << as.number << func(); break;
				case ValueType::Bool:      std::cout << (as.boolean ? "true" : "false") << func(); break;
				case ValueType::Character: std::cout << as.character << func(); break;
				case ValueType::Null:      std::cout << "null" << func(); break;
				case ValueType::Obj: {
					switch (as.object->type) {
						case ObjType::String:
							std::cout << as_string()->obj << func();
							break;
					}
				}
			}
		}

		bool operator==(const Value& other) const {
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
					}
				}
			}

			// unreachable
			__debugbreak();
			return false;
		}
	};


}
