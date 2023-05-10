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

#define DEBUG_PRINT_CODE 1
#define DEBUG_TRACE_EXECUTION 1

	static void repl();
	static InterpretResult run(const std::string& filepath, const std::string& source);
	static void run_file(const std::string& filepath);

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
		std::string line;
		for (;;) {
			printf(">> ");
			std::getline(std::cin, line);

			InterpretResult result = run("stdin", line);

			line.clear();
		}
	}

	static InterpretResult run(const std::string& filepath, const std::string& source)
	{
		Compiler compiler(filepath, source);
		
		ByteBlock byte_code;
		if (!compiler.compile(&byte_code)) {
			std::cerr << compiler.get_last_error();
			return InterpretResult::CompileError;
		}

		VirtualMachine vm;
		return vm.interpret(&byte_code);
	}

	static void run_file(const std::string& filepath)
	{
		std::ifstream file(filepath);
		if (!file.is_open()) {
			std::cerr << "Failed to open file '/" << filepath << "'\n";
			return;
		}

		file.seekg(0, std::ios::end);
		size_t file_size = file.tellg();
		file.seekg(0, std::ios::beg);
		std::string source;
		source.resize(file_size);
		file.read(source.data(), file_size);

		run(filepath, source);
	}

	static void print_value(Value value)
	{
		printf("%g", value);
	}

}