// src/ast.cpp
#include "ast.hpp"
#include "ir_builder.hpp"
#include <fmt/core.h>
#include <ranges>
#include <string>

std::string indent(int d) { return std::string(d * 2, ' '); }

/**
 * @brief dump implementation
 *
 */

namespace ast {
void CompUnitAST::dump(int depth) const {
  fmt::println("{}CompUnitAST:", indent(depth));
  if (func_def) {
    func_def->dump(depth + 1);
  }
}

void FuncDefAST::dump(int depth) const {
  fmt::println("{}FuncDefAST: {}", indent(depth), ident);
  if (func_type) {
    func_type->dump(depth + 1);
  }
  if (block) {
    block->dump(depth + 1);
  }
}

void FuncTypeAST::dump(int depth) const {
  fmt::println("{}FuncTypeAST: {}", indent(depth), type);
}

void BlockAST::dump(int depth) const {
  fmt::println("{}BlockAST:", indent(depth));
  if (stmt) {
    stmt->dump(depth + 1);
  }
}

void StmtAST::dump(int depth) const {
  fmt::println("{}StmtAST(return):", indent(depth));
  if (number) {
    number->dump(depth + 1);
  }
}

void NumberAST::dump(int depth) const {
  fmt::println("{}NumberAST: {}", indent(depth), num);
}
} // namespace ast

/**
 * @brief codegen implementation
 *
 */

namespace ast {
void CompUnitAST::codeGen(ir::KoopaBuilder &builder) const {
  if (func_def) {
    func_def->codeGen(builder);
  }
}

void FuncDefAST::codeGen(ir::KoopaBuilder &builder) const {
  builder.append("fun @");
  builder.append(ident);
  builder.append("(): ");
  if (func_type) {
    func_type->codeGen(builder);
  }
  if (block) {
    block->codeGen(builder);
  }
}

void FuncTypeAST::codeGen(ir::KoopaBuilder &builder) const {
  builder.append("i32 ");
}

void BlockAST::codeGen(ir::KoopaBuilder &builder) const {
  builder.append("{\n%entry:\n");
  if (stmt) {
    stmt->codeGen(builder);
  }
  builder.append("}\n");
}

void StmtAST::codeGen(ir::KoopaBuilder &builder) const {
  builder.append("   ret ");
  if (number) {
    number->codeGen(builder);
  }
  builder.append("\n");
}

void NumberAST::codeGen(ir::KoopaBuilder &builder) const {
  builder.append(std::to_string(num));
}
} // namespace ast