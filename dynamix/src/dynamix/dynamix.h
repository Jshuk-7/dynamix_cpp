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
	static void run(const std::string& filepath, const std::string& source);
	static void run(VirtualMachine& vm, const std::string& filepath, const std::string& source);
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
		VirtualMachine vm;

		for (;;) {
			printf(">> ");
			std::string line;
			std::getline(std::cin, line);
			run(vm, "stdin", line);
		}
	}

	static void run(VirtualMachine& vm, const std::string& filepath, const std::string& source)
	{
		Compiler compiler(filepath, source);
		
		std::string line;
		std::vector<std::string> lines;
		std::stringstream ss(source);
		while (std::getline(ss, line, '\n'))
			lines.push_back(line);

		ByteBlock byte_code(lines);
		if (!compiler.compile(&byte_code)) {
			std::cerr << compiler.get_last_error();
			return;
		}

		if (vm.interpret(&byte_code) == InterpretResult::RuntimeError) {
			const RuntimeError& error = vm.get_last_error();
			std::cerr << std::format(
				"thread 'main' panicked at: {}\n<{}:{}> Runtime Error: {}",
				error.source,
				filepath,
				error.line,
				error.msg
			);
			return;
		}
	}

	static void run(const std::string& filepath, const std::string& source)
	{
		VirtualMachine vm;
		run(vm, filepath, source);
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

}