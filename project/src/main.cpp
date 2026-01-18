/**
 * @file main.cpp
 * @brief Entry point for the SysY compiler.
 *
 * The compiler pipeline consists of:
 * 1. Lexing & Parsing (Flex/Bison) -> AST
 * 2. IR Generation (AST::codeGen) -> Koopa IR
 * 3. Backend (TargetCodeGen::visit) -> RISC-V Assembly
 */

#include "Log/log.hpp"
#include "backend/backend.hpp"
#include "backend/koopawrapper.hpp"
#include "ir/ast.hpp"
#include "ir/ir_builder.hpp"
#include <cassert>
#include <cstdio>
#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/os.h>
#include <memory>
#include <string>
#include <vector>

// for lexer & bison
extern FILE *yyin;
extern int yyparse(std::unique_ptr<ast::BaseAST> &ast);

struct Config {
  std::string mode;
  std::string input_file;
  std::string output_file;
};

auto helpMessage() -> void {
  fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::cyan), "Usage: ");
  fmt::print("./compiler [options] <input_file>\n\n");

  fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::cyan), "Options: \n");

  // clang-format off
  fmt::print("  {:<16} {}\n", "-h, --help", "Show this help message and exit");
  fmt::print("  {:<16} {}\n", "-koopa", "Compile SysY to Koopa IR");
  fmt::print("  {:<16} {}\n", "-riscv", "Compile SysY to RISC-V assembly");
  fmt::print("  {:<16} {}\n", "-perf", "Compile with performance optimizations");
  fmt::print("  {:<16} {}\n", "-o <file>", "Place the output into <file>");
  // clang-format on

  fmt::print(fmt::emphasis::bold,
             "\nExample: ./compiler -koopa hello.c -o hello.koopa\n");
};

auto parseArgs(int argc, const char *argv[]) -> Config {
  if (argc != 2 && argc != 5) {
    helpMessage();
    Log::panic("The number of input parameters must be five or two.");
  }

  std::vector<std::string_view> Args(argv + 1, argv + argc);
  Config config;

  for (int i = 0; i < ssize(Args); ++i) {

    if (Args[i] == "-h" || Args[i] == "--help") {
      helpMessage();
      exit(0);
    }

    if (Args[i] == "-koopa" || Args[i] == "-riscv" || Args[i] == "-perf") {
      config.mode = Args[i];
    } else if (Args[i] == "-o" && i + 1 < ssize(Args)) {
      config.output_file = Args[++i];
    } else {
      config.input_file = Args[i];
    }
  }

  return config;
}

// ./compiler -koopa input_sources -o elf-file
auto main(int argc, const char *argv[]) -> int {
  auto config = parseArgs(argc, argv);

  // 1. Initialize Lexer and Parse SysY source into AST
  yyin = fopen(config.input_file.c_str(), "r");
  if (!yyin) {
    Log::panic("Invalid input!");
  }

  std::unique_ptr<ast::BaseAST> ast;
  auto ret = yyparse(ast);
  if (ret) {
    Log::panic("Parsing failed!");
  }

  // Debug: Dump AST
  fmt::print(fmt::fg(fmt::color::cyan), "[Success] Parse ast succeed!\n\n");
  ast->dump(0);

  // 2. Generate Koopa IR from AST
  ir::KoopaBuilder irBuilder;
  ast->codeGen(irBuilder);

  const std::string ir = irBuilder.build();
  if (config.mode == "-koopa") {
    auto out = fmt::output_file(config.output_file);
    out.print("{}", ir);
    fmt::print(fmt::fg(fmt::color::cyan), "[Success] Parse koopa succeed!\n");
  }

  // 3. Generate RISC-V assembly from Koopa IR
  backend::KoopaWrapper wrapper(ir);
  backend::TargetCodeGen generator;
  generator.visit(wrapper.getRaw());

  const std::string asmCode = generator.getAssembly();
  if (config.mode == "-riscv") {
    auto out = fmt::output_file(config.output_file);
    out.print("{}", asmCode);
    fmt::print(fmt::fg(fmt::color::cyan), "[Success] Parse riscv succeed!\n");
  }

  fclose(yyin);
  return 0;
}