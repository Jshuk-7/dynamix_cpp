#include "VirtualMachine.h"

#include "Disassembler.h"
#include "dynamix.h"

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

		for (;;) {
#if DEBUG_TRACE_EXECUTION
			printf("          ");
			if (!m_Stack.is_empty()) {
				uint32_t i = 0;
				while (i < m_Stack.size()) {
					Value* slot = m_Stack.first() + (i++);
					printf("[ ");
					print_value(*slot);
					printf(" ]");
				}
			}
			printf("\n");

			Disassembler::disassemble_instruction(m_Block, (int32_t)(m_Ip - m_Block->bytes.data()));
#endif

			OpCode instruction;
			switch (instruction = (OpCode)READ_BYTE()) {
				case OpCode::Constant: {
					Value constant = READ_CONSTANT();
					m_Stack.push(constant);
				} break;
				case OpCode::Pop: {
					Value constant = m_Stack.pop();
				} break;
				case OpCode::Return: {
					print_value(m_Stack.pop());
					printf("\n");
					return InterpretResult::Ok;
				}
			}
		}

#undef READ_CONSTANT
#undef READ_BYTE
	}

	void VirtualMachine::reset_stack()
	{
		m_Stack.clear();
	}

}
