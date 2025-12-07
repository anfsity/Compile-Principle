// src/main.cpp
#include <cassert>
#include <memory>
#include <cstdio>
#include <string>
#include "ast.hpp"
#include <fmt/core.h>
#include <fmt/os.h>

extern FILE *yyin;
extern int yyparse(std::unique_ptr<ast::BaseAST> &ast);

int main(int argc, const char *argv[]) {
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
  ast->Dump(0);

  std::string ir;
  ast->CodeGen(ir);
  auto out = fmt::output_file(outputFile);
  out.print("{}", ir);

  fclose(yyin);
  return 0;
}