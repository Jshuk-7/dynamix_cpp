#pragma once

#include "VirtualMachine.h"

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>

namespace dynamix {

#define DEBUG_STACK_TRACE 0
#define DEBUG_DISASSEMBLE_CODE 0

	static void repl();
	static InterpretResult run(const std::string& filepath, const std::string& source);
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
			vm.run_code("stdin", line);
		}
	}

	static InterpretResult run(const std::string& filepath, const std::string& source)
	{
		VirtualMachine vm;
		return vm.run_code(filepath, source);
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