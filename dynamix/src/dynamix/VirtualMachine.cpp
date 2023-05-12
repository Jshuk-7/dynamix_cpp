#include "VirtualMachine.h"

#include "dynamix.h"
#include "Value.h"
#include "Object.h"
#include "Compiler.h"
#include "Disassembler.h"

#include <sstream>
#include <iomanip>

namespace dynamix {

#define CALL_FRAME_CAPACITY 64
#define STACK_CAPACITY (CALL_FRAME_CAPACITY * UINT8_MAX + 1)
#define OBJECT_CAPACITY 256

	VirtualMachine::VirtualMachine()
	{
		m_Stack.reserve(STACK_CAPACITY);
		m_Frames.reserve(CALL_FRAME_CAPACITY);
		m_Objects.reserve(OBJECT_CAPACITY);
	}

	VirtualMachine::~VirtualMachine()
	{
		const size_t obj_count = m_Objects.size();
		for (size_t i = 0; i < obj_count; i++) {
			Maybe<Obj*> value = m_Objects.pop();
			if (!value.is_some()) {
				continue;
			}

			Obj* obj = value.data();
			delete obj;
		}
	}

	InterpretResult VirtualMachine::run_code(const std::string& filepath, const std::string& source)
	{
		Compiler compiler(filepath, source);

		ObjFunction* function = compiler.compile();
		if (!function) {
			std::cerr << (!is_repl_mode ? std::format("failed to compile program '{}'\n", filepath) : "")
				<< compiler.get_last_error();

			return InterpretResult::CompileError;
		}

		m_Stack.push(Value(function));

		CallFrame frame;
		frame.function = function;
		frame.ip = function->block.bytes.data();
		frame.slots = &m_Stack[0];
		m_Frames.push(frame);

		if (interpret() == InterpretResult::RuntimeError) {
			std::cerr << std::format(
				"thread 'main' panicked at: '{}'\n<{}:{}:{}> Runtime Error: {}\n",
				m_LastError.source,
				filepath,
				m_LastError.line,
				m_LastError.function_name,
				m_LastError.msg
			);

			return InterpretResult::RuntimeError;
		}

		return InterpretResult::Ok;
	}

	InterpretResult VirtualMachine::interpret()
	{
		CallFrame* frame = &m_Frames[m_Frames.size() - 1];

#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() (frame->function->block.constants[READ_BYTE()])
#define READ_STRING() (READ_CONSTANT().as_string())
#define TYPE_MISMATCH(lhs, rhs, op)\
			auto lhs_type = value_type_to_string(lhs.type, lhs.is_object() ? &lhs.as.object->type : nullptr);\
			auto rhs_type = value_type_to_string(rhs.type, rhs.is_object() ? &rhs.as.object->type : nullptr);\
			runtime_error(std::format("operator '{}' not defined for types '{}' and '{}'", op, lhs_type, rhs_type), frame)
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

#if DEBUG_STACK_TRACE
		printf("-- stack trace --\n");
#endif
		for (;;) {
#if DEBUG_STACK_TRACE
			printf("          ");
			for (size_t i = 0; i < m_Stack.size(); i++) {
				const Value& slot = m_Stack[i];
				printf("[ ");
				slot.print(false);
				printf(" ]");
			}
			printf("\n");

			Disassembler::disassemble_instruction(
				&frame->function->block,
				(int32_t)(frame->ip - frame->function->block.bytes.data())
			);
#endif

			switch (OpCode instruction = (OpCode)READ_BYTE()) {
				case OpCode::PushConstant: {
					Value constant = READ_CONSTANT();
					if (constant.is_object()) {
						m_Objects.push(constant.as.object);
					}

					m_Stack.push(constant);
				} break;
				case OpCode::Pop: m_Stack.pop(); break;
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
					auto error = [&]() {
						TYPE_MISMATCH(peek(1), peek(), '+');
						return InterpretResult::RuntimeError;
					};

					if (peek(1).is_string()) {
						bool failed = false;
						concatenate(failed);

						if (failed) {
							return error();
						}
					}
					else if (peek(1).is(ValueType::Number) && peek().is(ValueType::Number)) {
						double b = m_Stack.pop().data().as.number;
						double a = m_Stack.pop().data().as.number;
						m_Stack.push(Value(a + b));
					}
					else {
						return error();
					}
				} break;
				case OpCode::Sub:     BINARY_OP(-, '-'); break;
				case OpCode::Mul:     BINARY_OP(*, '*'); break;
				case OpCode::Div:     BINARY_OP(/, '/'); break;
				case OpCode::Negate: {
					if (!peek(0).is(ValueType::Number)) {
						runtime_error("operand must be a number", frame);
						return InterpretResult::RuntimeError;
					}

					m_Stack.push(Value(-m_Stack.pop().data().as.number));
				} break;
				case OpCode::Not: m_Stack.push(Value(is_falsey(m_Stack.pop().data()))); break;
				case OpCode::Jmp: {
					uint16_t offset = READ_SHORT();
					frame->ip += offset;
				} break;
				case OpCode::Jz: {
					uint16_t offset = READ_SHORT();

					if (is_falsey(peek())) {
						frame->ip += offset;
					}
				} break;
				case OpCode::Loop: {
					uint16_t offset = READ_SHORT();
					frame->ip -= offset;
				} break;
				case OpCode::DefineGlobal: {
					ObjString* name = READ_STRING();
					if (m_Globals.contains(name->obj)) {
						runtime_error(std::format(
							"global variable '{}' has multiple definitions; multiple initialization",
							name->obj
						), frame);
						return InterpretResult::RuntimeError;
					}

					m_Globals[name->obj] = peek();
					m_Stack.pop();
				} break;
				case OpCode::GetGlobal: {
					ObjString* name = READ_STRING();
					if (!m_Globals.contains(name->obj)) {
						std::string err = std::format("undefined variable '{}'", name->obj);
						runtime_error(err, frame);
						return InterpretResult::RuntimeError;
					}

					Value value = m_Globals[name->obj];
					m_Stack.push(value);
				} break;
				case OpCode::SetGlobal: {
					ObjString* name = READ_STRING();
					if (!m_Globals.contains(name->obj)) {
						std::string err = std::format("undefined variable '{}'", name->obj);
						runtime_error(err, frame);
						return InterpretResult::RuntimeError;
					}

					m_Globals[name->obj] = peek();
				} break;
				case OpCode::GetLocal: {
					uint8_t slot = READ_BYTE();
					m_Stack.push(frame->slots[slot]);
				} break;
				case OpCode::SetLocal: {
					uint8_t slot = READ_BYTE();
					frame->slots[slot] = peek();
				} break;
				case OpCode::Print: m_Stack.pop().data().print(true); break;
				case OpCode::Return: return InterpretResult::Ok;
				default: {
					size_t opcode = frame->ip - frame->function->block.bytes.data() - 1;
					runtime_error(std::format(
						"OpCode '{}' not implemented in virtual machine",
						opcode
					), frame);
					return InterpretResult::RuntimeError;
				}
			}
		}

#undef BINARY_OP
#undef TYPE_MISMATCH
#undef READ_STRING
#undef READ_CONSTANT
#undef READ_SHORT
#undef READ_BYTE
	}

	Value VirtualMachine::peek(int32_t distance) const
	{
		return m_Stack[m_Stack.size() - 1 - distance];
	}

	void VirtualMachine::reset_stack()
	{
		m_Stack.clear();
		m_Frames.clear();
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

	void VirtualMachine::concatenate(bool& failed)
	{
		ObjString* result = new ObjString();

		if (peek().is_string()) {
			ObjString* rhs = m_Stack.pop().data().as_string();
			ObjString* lhs = m_Stack.pop().data().as_string();

			std::string string = lhs->obj;
			remove_null_terminator(string);
			std::string res = string + rhs->obj + '\0';

			result->obj = res;
		}
		else if (peek().is(ValueType::Character)) {
			char rhs = m_Stack.pop().data().as.character;
			ObjString* lhs = m_Stack.pop().data().as_string();

			std::string string = lhs->obj;
			remove_null_terminator(string);
			std::string res = string + rhs + '\0';

			result->obj = res;
		}
		else if (peek().is(ValueType::Number)) {
			double rhs = m_Stack.pop().data().as.number;
			ObjString* lhs = m_Stack.pop().data().as_string();

			std::string string = lhs->obj;
			remove_null_terminator(string);
			std::stringstream ss;
			ss << string << std::setw(1) << rhs << '\0';
			std::string res = ss.str();

			result->obj = res;
		}
		else {
			failed = true;
		}

		((Obj*)result)->type = ObjType::String;

		m_Objects.push((Obj*)result);
		m_Stack.push(Value((Obj*)result));
	}

	void VirtualMachine::remove_null_terminator(std::string& str)
	{
		while (size_t pos = str.find('\0')) {
			if (pos == std::string::npos) {
				return;
			}

			str.erase(pos);
		}
	}

	void VirtualMachine::runtime_error(const std::string& error, const CallFrame* frame)
	{
		size_t instruction = frame->ip - frame->function->block.bytes.data() - 1;
		uint32_t line = frame->function->block.lines[instruction];
		//std::string source = m_Block->source_lines[line - 1];
		std::string function_name;

		if (frame->function->name.empty()) {
			function_name = "<script>";
		}
		else {
			function_name = frame->function->name;
		}

		m_LastError = RuntimeError{ error, "", function_name, line };
		reset_stack();
	}

}
