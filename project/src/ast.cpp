// src/ast.cpp
#include "ast.hpp"
#include <fmt/core.h>
#include <ranges>
#include <string>

std::string indent(int d) { return std::string(d * 2, ' '); }

void ast::CompUnitAST::Dump(int depth) const {
  fmt::println("{}CompUnitAST:", indent(depth));
  if (func_def) {
    func_def->Dump(depth + 1);
  }
}

void ast::CompUnitAST::CodeGen(std::string &ir) const {
  if (func_def) {
    func_def->CodeGen(ir);
  }
}

void ast::FuncDefAST::Dump(int depth) const {
  fmt::println("{}FuncDefAST: {}", indent(depth), ident);
  if (func_type) {
    func_type->Dump(depth + 1);
  }
  if (block) {
    block->Dump(depth + 1);
  }
}

void ast::FuncDefAST::CodeGen(std::string &ir) const {
  ir += "fun @" + ident + "(): ";
  if (func_type) {
    func_type->CodeGen(ir);
  }
  if (block) {
    block->CodeGen(ir);
  }
}

void ast::FuncTypeAST::Dump(int depth) const {
  fmt::println("{}FuncTypeAST: {}", indent(depth), type);
}

void ast::FuncTypeAST::CodeGen(std::string &ir) const {
  ir += "i32 ";
}

void ast::BlockAST::Dump(int depth) const {
  fmt::println("{}BlockAST:", indent(depth));
  if (stmt) {
    stmt->Dump(depth + 1);
  }
}

void ast::BlockAST::CodeGen(std::string &ir) const {
  ir += "{\n%entry:\n";
  if (stmt) {
    stmt->CodeGen(ir);
  }
  ir += "}";
}

void ast::StmtAST::Dump(int depth) const {
  fmt::println("{}StmtAST(return):", indent(depth));
  if (number) {
    number->Dump(depth + 1);
  }
}

void ast::StmtAST::CodeGen(std::string &ir) const {
  ir += "   ret ";
  if (number) {
    number->CodeGen(ir);
  }
  ir += "\n";
}

void ast::NumberAST::Dump(int depth) const {
  fmt::println("{}NumberAST: {}", indent(depth), num);
}

void ast::NumberAST::CodeGen(std::string &ir) const {
  ir += std::to_string(num);
}