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

	struct RuntimeError
	{
		std::string msg;
		std::string source;
		uint32_t line;
	};

	class VirtualMachine
	{
	public:
		VirtualMachine();

		InterpretResult interpret(ByteBlock* block);
		InterpretResult run();

		const RuntimeError& get_last_error() const;

	private:
		Value peek(int32_t distance = 0) const;
		void reset_stack();
		bool is_falsey(Value value) const;
		void runtime_error(const std::string& error);

	private:
		Stack<Value> m_Stack;
		ByteBlock* m_Block = nullptr;
		uint8_t* m_Ip = nullptr;
		RuntimeError m_LastError;
	};

}
