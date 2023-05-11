#include "VirtualMachine.h"

#include "Disassembler.h"
#include "dynamix.h"

#include <sstream>
#include <iomanip>

namespace dynamix {

#define STACK_CAPACITY 256
#define OBJECT_CAPACITY 256

	VirtualMachine::VirtualMachine()
	{
		m_Stack.reserve(STACK_CAPACITY);
		m_Objects.reserve(OBJECT_CAPACITY);
	}

	VirtualMachine::~VirtualMachine()
	{
		const size_t obj_count = m_Objects.size();
		for (uint32_t i = 0; i < obj_count; i++) {
			Maybe<Obj*> value = m_Objects.pop();
			if (!value.is_some()) {
				continue;
			}

			Obj* obj = value.data();
			delete obj;
		}
	}

	InterpretResult VirtualMachine::interpret(ByteBlock* block)
	{
		m_Block = block;
		m_Ip = m_Block->bytes.data();

		return run();
	}

	InterpretResult VirtualMachine::run()
	{
#define READ_BYTE() (*m_Ip++)
#define READ_CONSTANT() (m_Block->constants[READ_BYTE()])
#define TYPE_MISMATCH(lhs, rhs, op)\
			auto lhs_type = value_type_to_string(lhs.type, lhs.is_object() ? &lhs.as.object->type : nullptr);\
			auto rhs_type = value_type_to_string(rhs.type, rhs.is_object() ? &rhs.as.object->type : nullptr);\
			runtime_error(std::format("operator '{}' not defined for types '{}' and '{}'", op, lhs_type, rhs_type))
#define BINARY_OP(op, op_char)\
			do {\
				if (peek(1).type != peek().type) {\
					TYPE_MISMATCH(peek(1), peek(), op_char);\
					return InterpretResult::RuntimeError;\
				}\
				switch (peek().type) {\
					case ValueType::Number: {\
						double b = m_Stack.pop().data().as.number;\
						double a = m_Stack.pop().data().as.number;\
						m_Stack.push(Value(a op b));\
					} break;\
					default:\
						TYPE_MISMATCH(peek(1), peek(), op_char);\
						return InterpretResult::RuntimeError;\
				}\
			} while (false)

		for (;;) {
#if DEBUG_TRACE_EXECUTION
			printf("          ");
			if (!m_Stack.is_empty()) {
				uint32_t i = 0;
				while (i < m_Stack.size()) {
					Value* slot = m_Stack.first() + (i++);
					printf("[ ");
					(*slot).print(false);
					printf(" ]");
				}
			}
			printf("\n");

			Disassembler::disassemble_instruction(m_Block, (int32_t)(m_Ip - m_Block->bytes.data()));
#endif

			switch (OpCode instruction = (OpCode)READ_BYTE()) {
				case OpCode::Constant: {
					Value constant = READ_CONSTANT();
					if (constant.is_object()) {
						m_Objects.push(constant.as.object);
					}

					m_Stack.push(constant);
				} break;
				case OpCode::Null: m_Stack.push(Value(nullptr)); break;
				case OpCode::True: m_Stack.push(Value(true)); break;
				case OpCode::False: m_Stack.push(Value(false)); break;
				case OpCode::Equal: {
					Value b = m_Stack.pop().data();
					Value a = m_Stack.pop().data();
					m_Stack.push(Value(a == b));
				} break;
				case OpCode::Greater: BINARY_OP(>, '>'); break;
				case OpCode::Less:    BINARY_OP(<, '<'); break;
				case OpCode::Add: {
					if (peek(1).is_string()) {
						concatenate();
					}
					else if (peek(1).is(ValueType::Number) && peek().is(ValueType::Number)) {
						double b = m_Stack.pop().data().as.number;
						double a = m_Stack.pop().data().as.number;
						m_Stack.push(Value(a + b));
					}
					else {
						TYPE_MISMATCH(peek(1), peek(), '+');
						return InterpretResult::RuntimeError;
					}
				} break;
				case OpCode::Sub:     BINARY_OP(-, '-'); break;
				case OpCode::Mul:     BINARY_OP(*, '*'); break;
				case OpCode::Div:     BINARY_OP(/, '/'); break;
				case OpCode::Negate: {
					if (!peek(0).is(ValueType::Number)) {
						runtime_error("operand must be a number");
						return InterpretResult::RuntimeError;
					}

					m_Stack.push(Value(-m_Stack.pop().data().as.number));
				} break;
				case OpCode::Not: m_Stack.push(Value(is_falsey(m_Stack.pop().data()))); break;
				case OpCode::Print: m_Stack.pop().data().print(true); break;
				case OpCode::Pop: m_Stack.pop(); break;
				case OpCode::Return: return InterpretResult::Ok;
			}
		}

#undef TYPE_MISMATCH
#undef BINARY_OP
#undef READ_CONSTANT
#undef READ_BYTE
	}

	const RuntimeError& VirtualMachine::get_last_error() const
	{
		return m_LastError;
	}

	Value VirtualMachine::peek(int32_t distance) const
	{
		return m_Stack[(int32_t)m_Stack.size() - 1 - distance];
	}

	void VirtualMachine::reset_stack()
	{
		m_Stack.clear();
	}

	bool VirtualMachine::is_falsey(Value value) const
	{
		switch (value.type) {
			case ValueType::Number:    return value.as.number == 0.0;
			case ValueType::Bool:      return value.as.boolean == false;
			case ValueType::Character: return value.as.character == '0';
			case ValueType::Null:      return true;
			case ValueType::Obj: {
				switch (value.as.object->type)
				{
					case ObjType::String: return value.as_string()->obj.empty(); break;
				}
			}
		}

		// unreachable
		__debugbreak();
		return false;
	}

	void VirtualMachine::concatenate()
	{
		ObjString* result = new ObjString();
		((Obj*)result)->type = ObjType::String;

		if (peek().is_string()) {
			ObjString* b = m_Stack.pop().data().as_string();
			ObjString* a = m_Stack.pop().data().as_string();

			std::string string = a->obj;
			string += b->obj;

			result->obj = string;
		}
		else if (peek().is(ValueType::Character)) {
			char c = m_Stack.pop().data().as.character;
			ObjString* lhs = m_Stack.pop().data().as_string();

			std::string string = lhs->obj;
			string += c + '\0';

			result->obj = string;
		}
		else if (peek().is(ValueType::Number)) {
			double n = m_Stack.pop().data().as.number;
			ObjString* lhs = m_Stack.pop().data().as_string();

			std::string string = lhs->obj;
			string.pop_back();
			std::stringstream ss;
			ss << string << std::setw(1) << n << '\0';
			string = ss.str();

			result->obj = string;
		}

		m_Objects.push((Obj*)result);
		m_Stack.push(Value((Obj*)result));
	}

	void VirtualMachine::runtime_error(const std::string& error)
	{
		size_t instruction = m_Ip - m_Block->bytes.data() - 1;
		uint32_t line = m_Block->lines[instruction];
		std::string source = m_Block->source_lines[line - 1];

		m_LastError = RuntimeError{ error, source, line };
		reset_stack();
	}

}
