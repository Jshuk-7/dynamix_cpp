#pragma once

#include "ByteBlock.h"
#include "Disassembler.h"
#include "VirtualMachine.h"
#include "Compiler.h"

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>

namespace dynamix {

#define DEBUG_STACK_TRACE 0
#define DEBUG_DISASSEMBLE_CODE 0

	static void repl();
	static InterpretResult run(const std::string& filepath, const std::string& source);
	static InterpretResult run(VirtualMachine& vm, const std::string& filepath, const std::string& source);
	static InterpretResult run_file(const std::string& filepath);

	static bool is_repl_mode = false;

	static void runtime_start(int argc, char* argv[])
	{
		if (argc == 1) {
			repl();
		}
		else if (argc == 2) {
			run_file(argv[1]);
		}
		else {
			std::cout << "Usage: dynamix <script>\n";
		}

		std::cin.get();
	}

	static void repl()
	{
		is_repl_mode = true;
		VirtualMachine vm;

		for (;;) {
			printf(">> ");
			std::string line;
			std::getline(std::cin, line);
			run(vm, "stdin", line);
		}
	}

	static InterpretResult run(VirtualMachine& vm, const std::string& filepath, const std::string& source)
	{
		Compiler compiler(filepath, source);
		
		std::string line;
		std::vector<std::string> lines;
		std::stringstream ss(source);
		while (std::getline(ss, line, '\n'))
			lines.push_back(line);

		ByteBlock byte_code(lines);
		if (!compiler.compile(&byte_code)) {
			std::cerr << (!is_repl_mode ? std::format("failed to compile program '{}'\n", filepath) : "")
				<< compiler.get_last_error();

			return InterpretResult::CompileError;
		}

		if (vm.interpret(&byte_code) == InterpretResult::RuntimeError) {
			const RuntimeError& error = vm.get_last_error();
			std::cerr << std::format(
				"thread 'main' panicked at: {}\n<{}:{}> Runtime Error: {}\n",
				error.source,
				filepath,
				error.line,
				error.msg
			);
			return InterpretResult::RuntimeError;
		}

		return InterpretResult::Ok;
	}

	static InterpretResult run(const std::string& filepath, const std::string& source)
	{
		VirtualMachine vm;
		return run(vm, filepath, source);
	}

	static InterpretResult run_file(const std::string& filepath)
	{
		std::ifstream file(filepath);
		if (!file.is_open()) {
			std::cerr << "Failed to open file '/" << filepath << "'\n";
			return InterpretResult::FailedToOpenFile;
		}

		file.seekg(0, std::ios::end);
		size_t file_size = file.tellg();
		file.seekg(0, std::ios::beg);
		std::string source;
		source.resize(file_size);
		file.read(source.data(), file_size);

		InterpretResult result = run(filepath, source);
		if (result == InterpretResult::Ok) {
			printf("program exited successfully...");
		}

		return result;
	}

}