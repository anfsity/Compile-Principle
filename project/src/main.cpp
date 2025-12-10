// src/main.cpp
#include "ast.hpp"
#include "backend.hpp"
#include "ir_builder.hpp"
#include <cassert>
#include <cstdio>
#include <fmt/core.h>
#include <fmt/os.h>
#include <memory>
#include <string>

extern FILE *yyin;
extern int yyparse(std::unique_ptr<ast::BaseAST> &ast);

auto main(int argc, const char *argv[]) -> int {
  assert(argc == 5 && "There must be five parameters.");

  // auto mode = *(argv + 1);
  auto inputFile = *(argv + 2);
  auto outputFile = *(argv + 4);

  yyin = fopen(inputFile, "r");
  assert(yyin && "invaild input");

  std::unique_ptr<ast::BaseAST> ast;
  auto ret = yyparse(ast);
  assert(!ret && "parsing failed");

  fmt::println("parsing successed! there is a AST:");
  ast->dump(0);

  ir::KoopaBuilder irBuilder;
  ast->codeGen(irBuilder);

  const std::string ir = irBuilder.build();
  auto out = fmt::output_file(outputFile);
  out.print("{}", ir);

  // backend::KoopaWrapper wrapper(ir);
  // backend::TargetCodeGen generator;
  // generator.visit(wrapper.getRaw());

  // const std::string asmCode = generator.getAssembly();
  // auto asmOut = fmt::output_file("debug/hello.asm");
  // asmOut.print("{}", asmCode);
  // auto out = fmt::output_file(outputFile);
  // out.print("{}", asmCode);

  fclose(yyin);
  return 0;
}