#pragma once

#include <iostream>

namespace dynamix {

	enum class ValueType
	{
		Number,
		Bool,
		Character,
		Null,
	};

	static const char* value_type_to_string(ValueType type) {
		switch (type) {
			case ValueType::Number:    return "number";
			case ValueType::Bool:      return "bool";
			case ValueType::Character: return "char";
			case ValueType::Null:      return "null";
		}

		__debugbreak();
		return "None";
	}

	struct Value
	{
		ValueType type;
		union
		{
			double number;
			bool boolean;
			char character;
		} as;

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

		bool is(ValueType _type) const {
			return type == _type;
		}

		void print(bool new_line) const {
			auto func = [&]() { return (new_line ? "\n" : ""); };

			switch (type) {
				case ValueType::Number:    std::cout << as.number << func(); break;
				case ValueType::Bool:      std::cout << (as.boolean ? "true" : "false") << func(); break;
				case ValueType::Character: std::cout << as.character << func(); break;
				case ValueType::Null:      std::cout << "null" << func(); break;
			}
		}
	};


}
