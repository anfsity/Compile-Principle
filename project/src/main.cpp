// src/main.cpp
#include "ir/ast.hpp"
#include "backend/backend.hpp"
#include "ir/ir_builder.hpp"
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

  bool output_bool = false;
  const std::string_view mode = *(argv + 1);
  auto inputFile = *(argv + 2);
  auto outputFile = *(argv + 4);

  yyin = fopen(inputFile, "r");
  assert(yyin && "invaild input");

  std::unique_ptr<ast::BaseAST> ast;
  auto ret = yyparse(ast);
  assert(!ret && "parsing failed");

  // generate abstract semantic tree
  fmt::println("parsing successed! there is a AST:");
  ast->dump(0);

  ir::KoopaBuilder irBuilder;
  ast->codeGen(irBuilder);

  const std::string ir = irBuilder.build();
  if (mode == "-koopa") {
    auto out = fmt::output_file(outputFile);
    out.print("{}", ir);
    output_bool = true;
  }

  // backend::KoopaWrapper wrapper(ir);
  // backend::TargetCodeGen generator;
  // generator.visit(wrapper.getRaw());

  // const std::string asmCode = generator.getAssembly();
  // if (mode == "-riscv") {
  //   auto out = fmt::output_file(outputFile);
  //   out.print("{}", asmCode);
  //   output_bool = true;
  // }

  assert(output_bool);

  fclose(yyin);
  return 0;
}