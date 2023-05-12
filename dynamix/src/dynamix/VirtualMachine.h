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
		std::string function_name;
		uint32_t line;
	};

	struct CallFrame
	{
		ObjFunction* function;
		uint8_t* ip;
		Value* slots;
	};

	class VirtualMachine
	{
	public:
		VirtualMachine();
		~VirtualMachine();

		InterpretResult run_code(const std::string& filepath, const std::string& source);

	private:
		InterpretResult interpret();

		Value peek(int32_t distance = 0) const;
		void reset_stack();
		bool is_falsey(Value value) const;
		void concatenate(bool& failed);
		void remove_null_terminator(std::string& str);
		void runtime_error(const std::string& error, const CallFrame* frame);

	private:
		/*uint8_t* m_Ip = nullptr;
		ByteBlock* m_Block = nullptr;*/
		
		Stack<Value> m_Stack;
		Stack<CallFrame> m_Frames;
		Stack<Obj*> m_Objects;
		
		std::unordered_map<std::string, Value> m_Globals;

		RuntimeError m_LastError;
	};

}
