#include "VirtualMachine.h"

#include "Disassembler.h"
#include "dynamix.h"

#include <cstdarg>

namespace dynamix {

#define STACK_CAPACITY 256

	VirtualMachine::VirtualMachine()
	{
		m_Stack.reserve(STACK_CAPACITY);
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
			auto lhs_type = value_type_to_string(lhs.type);\
			auto rhs_type = value_type_to_string(rhs.type);\
			runtime_error(std::format("operator '{}' not defined for types '{}' and '{}'", op, lhs_type, rhs_type))
#define BINARY_OP(op, op_char)\
			do {\
				if (peek(1).type != peek().type) {\
					TYPE_MISMATCH(peek(1), peek(), op_char);\
					return InterpretResult::RuntimeError;\
				}\
				switch (peek().type) {\
					case ValueType::Number: {\
						double b = m_Stack.pop().as.number;\
						double a = m_Stack.pop().as.number;\
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

			OpCode instruction;
			switch (instruction = (OpCode)READ_BYTE()) {
				case OpCode::Constant: m_Stack.push(READ_CONSTANT()); break;
				case OpCode::Null: m_Stack.push(Value(nullptr)); break;
				case OpCode::True: m_Stack.push(Value(true)); break;
				case OpCode::False: m_Stack.push(Value(false)); break;
				case OpCode::Equal: {
					Value b = m_Stack.pop();
					Value a = m_Stack.pop();
					m_Stack.push(Value(a == b));
				} break;
				case OpCode::Greater: BINARY_OP(>, '>'); break;
				case OpCode::Less:    BINARY_OP(<, '<'); break;
				case OpCode::Add:     BINARY_OP(+, '+'); break;
				case OpCode::Sub:     BINARY_OP(-, '-'); break;
				case OpCode::Mul:     BINARY_OP(*, '*'); break;
				case OpCode::Div:     BINARY_OP(/, '/'); break;
				case OpCode::Negate: {
					if (!peek(0).is(ValueType::Number)) {
						runtime_error("operand must be a number");
						return InterpretResult::RuntimeError;
					}

					m_Stack.push(Value(-m_Stack.pop().as.number));
				} break;
				case OpCode::Not: m_Stack.push(Value(is_falsey(m_Stack.pop()))); break;
				case OpCode::Pop: m_Stack.pop(); break;
				case OpCode::Return: {
					m_Stack.pop().print(true);
					return InterpretResult::Ok;
				}
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
		}

		// unreachable
		__debugbreak();
		return false;
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
