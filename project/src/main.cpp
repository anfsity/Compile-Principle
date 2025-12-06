#include <cassert>
#include <iostream>
#include <memory>
#include <cstdio>
#include <string>

extern FILE *yyin;
extern int yyparse(std::unique_ptr<std::string> &ast);

int main(int argc, const char *argv[]) {
  assert(argc == 5 && "There must be five parameters.");

  auto mode = *(argv + 1);
  auto inputFile = *(argv + 2);
  auto outputFile = *(argv + 4);

  yyin = fopen(inputFile, "r");
  assert(yyin && "invaild input");

  std::unique_ptr<std::string> ast;
  auto ret = yyparse(ast);
  assert(!ret && "parsing failed");

  std::cout << *ast << std::endl;
  return 0;
}