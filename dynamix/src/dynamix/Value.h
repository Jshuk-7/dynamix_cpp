#pragma once

#include <iostream>
#include <format>

namespace dynamix {

	enum class ValueType
	{
		Number,
		Bool,
		Character,
		Null,
		Obj,
	};

	enum class ObjType;
	typedef struct Obj Obj;
	typedef struct ObjFunction ObjFunction;
	typedef struct ObjString ObjString;

	const char* value_type_to_string(ValueType value_type, ObjType* obj_type = nullptr);

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

		bool is_object_type(ObjType type) const;
		bool is_function() const;
		bool is_string() const;

		ObjFunction* as_function() const;
		ObjString* as_string() const;

		void print(bool new_line) const;

		bool operator==(const Value& other) const;
	};


}
