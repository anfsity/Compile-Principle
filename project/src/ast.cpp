// src/ast.cpp
#include "ast.hpp"
#include "ir_builder.hpp"
#include <fmt/core.h>
#include <ranges>
#include <string>

using namespace ast;

std::string indent(int d) { return std::string(d * 2, ' '); }

std::string opToString(BinaryOp op) {
  switch (op) {
  case BinaryOp::Add:
    return "add";
  case BinaryOp::Sub:
    return "sub";
  case BinaryOp::Mul:
    return "mul";
  case BinaryOp::Div:
    return "div";
  case BinaryOp::Mod:
    return "mod";
  case BinaryOp::Lt:
    return "lt";
  case BinaryOp::Gt:
    return "gt";
  case BinaryOp::Le:
    return "le";
  case BinaryOp::Ge:
    return "ge";
  case BinaryOp::Eq:
    return "eq";
  case BinaryOp::Ne:
    return "ne";
  case BinaryOp::And:
    return "and";
  case BinaryOp::Or:
    return "or";
  default:
    return "?";
  }
}

/**
 * @brief dump implementation
 *
 */

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
  if (expr) {
    expr->dump(depth + 1);
  }
}

void NumberAST::dump(int depth) const {
  fmt::println("{}NumberAST: {}", indent(depth), val);
}

void UnaryExprAST::dump(int depth) const {
  std::string opStr = (op == UnaryOp::Neg) ? "neg" : "not";
  fmt::println("{}UnaryExprAST: {}", indent(depth), opStr);
  rhs->dump(depth + 1);
}

void BinaryExprAST::dump(int depth) const {
  fmt::println("{}BinaryExprAST: {}", indent(depth), opToString(op));
  lhs->dump(depth + 1);
  rhs->dump(depth + 1);
}

/**
 * @brief codegen implementation
 *
 */

std::string CompUnitAST::codeGen(ir::KoopaBuilder &builder) const {
  if (func_def) {
    func_def->codeGen(builder);
  }
  return "";
}

std::string FuncDefAST::codeGen(ir::KoopaBuilder &builder) const {
  builder.append("fun @");
  builder.append(ident);
  builder.append("(): ");
  if (func_type) {
    func_type->codeGen(builder);
  }
  if (block) {
    block->codeGen(builder);
  }
  return "";
}

std::string FuncTypeAST::codeGen(ir::KoopaBuilder &builder) const {
  std::string return_type;
  if (type == "int")
    return_type = "i32 ";
  else
    return_type = "void ";
  builder.append(return_type);
  return "";
}

std::string BlockAST::codeGen(ir::KoopaBuilder &builder) const {
  builder.append("{\n%entry:\n");
  if (stmt) {
    stmt->codeGen(builder);
  }
  builder.append("}\n");
  return "";
}

std::string StmtAST::codeGen(ir::KoopaBuilder &builder) const {
  std::string ret_val;
  if (expr) {
    ret_val = expr->codeGen(builder);
  }
  builder.append(fmt::format("  ret {}\n", ret_val));
  return "";
}

std::string
NumberAST::codeGen([[maybe_unused]] ir::KoopaBuilder &builder) const {
  return std::to_string(val);
}

std::string UnaryExprAST::codeGen(ir::KoopaBuilder &builder) const {
  std::string rhs_reg = rhs->codeGen(builder);
  std::string ret_reg = builder.newReg();
  switch (op) {
  case UnaryOp::Neg:
    builder.append(fmt::format("  {} = sub 0, {}\n", ret_reg, rhs_reg));
    break;
  case UnaryOp::Not:
    builder.append(fmt::format("  {} = eq 0, {}\n", ret_reg, rhs_reg));
    break;
  default:
    fmt::println(stderr, "Code Gen error: Unknown unary op");
    std::abort();
  }
  return ret_reg;
}

std::string BinaryExprAST::codeGen(ir::KoopaBuilder &builder) const {
  std::string lhs_reg = lhs->codeGen(builder);
  std::string rhs_reg = rhs->codeGen(builder);
  std::string ret_reg = builder.newReg();
  std::string ir_op = opToString(op);
  // FIXME: short-circuit evaluation not implemented
  if (op == BinaryOp::And || op == BinaryOp::Or) {
    std::string lhs_tmp = builder.newReg();
    std::string rhs_tmp = builder.newReg();

    builder.append(fmt::format("  {} = ne {}, 0\n", lhs_tmp, lhs_reg));
    builder.append(fmt::format("  {} = ne {}, 0\n", rhs_tmp, lhs_reg));
    lhs_reg = lhs_tmp;
    rhs_reg = rhs_tmp;
  }
  builder.append(
      fmt::format("  {} = {} {}, {}\n", ret_reg, ir_op, lhs_reg, rhs_reg));
  return ret_reg;
}