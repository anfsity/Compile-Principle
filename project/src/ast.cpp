// src/ast.cpp
#include <string>
#include <ranges>
#include <fmt/core.h>
#include "ast.hpp"
#include "ir_builder.hpp"

std::string indent(int d) { return std::string(d * 2, ' '); }

/**
 * @brief dump implementation
 * 
 */

namespace ast {
void CompUnitAST::Dump(int depth) const {
  fmt::println("{}CompUnitAST:", indent(depth));
  if (func_def) {
    func_def->Dump(depth + 1);
  }
}

void FuncDefAST::Dump(int depth) const {
  fmt::println("{}FuncDefAST: {}", indent(depth), ident);
  if (func_type) {
    func_type->Dump(depth + 1);
  }
  if (block) {
    block->Dump(depth + 1);
  }
}

void FuncTypeAST::Dump(int depth) const {
  fmt::println("{}FuncTypeAST: {}", indent(depth), type);
}

void BlockAST::Dump(int depth) const {
  fmt::println("{}BlockAST:", indent(depth));
  if (stmt) {
    stmt->Dump(depth + 1);
  }
}

void StmtAST::Dump(int depth) const {
  fmt::println("{}StmtAST(return):", indent(depth));
  if (number) {
    number->Dump(depth + 1);
  }
}

void NumberAST::Dump(int depth) const {
  fmt::println("{}NumberAST: {}", indent(depth), num);
}
} // namespace ast

/**
 * @brief codegen implementation
 * 
 */

namespace ast {
  void CompUnitAST::CodeGen(ir::KoopaBuilder &builder) const {
    if (func_def) {
      func_def->CodeGen(builder);
    }
  }

  void FuncDefAST::CodeGen(ir::KoopaBuilder &builder) const {
    builder.append("fun @");
    builder.append(ident);
    builder.append("(): ");
    if (func_type) {
      func_type->CodeGen(builder);
    }
    if (block) {
      block->CodeGen(builder);
    }
  }

  void FuncTypeAST::CodeGen(ir::KoopaBuilder &builder) const {
    builder.append("i32 ");
  }

  void BlockAST::CodeGen(ir::KoopaBuilder &builder) const {
    builder.append("{\n%entry:\n");
    if (stmt) {
      stmt->CodeGen(builder);
    }
    builder.append("}\n");
  }

  void StmtAST::CodeGen(ir::KoopaBuilder &builder) const {
    builder.append("   ret ");
    if (number) {
      number->CodeGen(builder);
    }
    builder.append("\n");
  }

  void NumberAST::CodeGen(ir::KoopaBuilder &builder) const {
    builder.append(std::to_string(num));
  }
}