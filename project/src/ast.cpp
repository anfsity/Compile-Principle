#include "ast.hpp"
#include <fmt/core.h>
#include <ranges>
#include <string>

std::string indent(int d) { return std::string(d * 2, ' '); }

void ast::CompUnitAST::Dump(int depth) const{
  fmt::println("{}CompUnitAST:", indent(depth));
  if (func_def) {
    func_def->Dump(depth + 1);
  }
}

void ast::FuncDefAST::Dump(int depth) const{
  fmt::println("{}FuncDefAST: {}", indent(depth), ident);
  if (func_type) {
    func_type->Dump(depth + 1);
  }
  if (block) {
    block->Dump(depth + 1);
  }
}

void ast::FuncTypeAST::Dump(int depth) const{
  fmt::println("{}FuncTypeAST: {}", indent(depth), type);
}

void ast::BlockAST::Dump(int depth) const{
  fmt::println("{}BlockAST:", indent(depth));
  if (stmt) {
    stmt->Dump(depth + 1);
  }
}

void ast::StmtAST::Dump(int depth) const{
  fmt::println("{}StmtAST(return):", indent(depth));
  if (number) {
    number->Dump(depth + 1);
  }
}

void ast::NumberAST::Dump(int depth) const{
  fmt::println("{}NumberAST: {}", indent(depth), num);
}