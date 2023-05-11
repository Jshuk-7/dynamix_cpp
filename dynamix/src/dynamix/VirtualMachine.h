#pragma once

#include "ByteBlock.h"
#include "Stack.h"
#include "Value.h"

#include <unordered_map>

namespace dynamix {

	enum class InterpretResult
	{
		Ok,
		CompileError,
		RuntimeError,
		FailedToOpenFile,
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
		~VirtualMachine();

		InterpretResult interpret(ByteBlock* block);
		InterpretResult run();

		const RuntimeError& get_last_error() const;

	private:
		Value peek(int32_t distance = 0) const;
		void reset_stack();
		bool is_falsey(Value value) const;
		void concatenate(bool& failed);
		void remove_null_terminator(std::string& str);
		void stack_trace();
		void runtime_error(const std::string& error);

	private:
		uint8_t* m_Ip = nullptr;
		ByteBlock* m_Block = nullptr;
		
		Stack<Value> m_Stack;
		Stack<Obj*> m_Objects;
		
		std::unordered_map<std::string, Value> m_Globals;

		RuntimeError m_LastError;
	};

}
