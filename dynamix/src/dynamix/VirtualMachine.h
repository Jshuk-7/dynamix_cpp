#pragma once

#include "ByteBlock.h"
#include "Stack.h"

namespace dynamix {

	enum class InterpretResult
	{
		Ok,
		CompileError,
		RuntimeError,
	};

	class VirtualMachine
	{
	public:
		VirtualMachine();

		InterpretResult interpret(ByteBlock* block);
		InterpretResult run();

	private:
		void reset_stack();
		bool is_falsey(Value value);

	private:
		Stack<Value> m_Stack;
		ByteBlock* m_Block = nullptr;
		uint8_t* m_Ip = nullptr;
	};

}
