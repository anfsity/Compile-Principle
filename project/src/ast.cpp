// src/ast.cpp
#include "ast.hpp"
#include "ir_builder.hpp"
#include <fmt/core.h>
#include <ranges>
#include <string>
#include <vector>

using namespace ast;

auto indent(int d) -> std::string { return std::string(d * 2, ' '); }

auto opToString(BinaryOp op) -> std::string {
  switch (op) {
  case BinaryOp::Add: return "add";
  case BinaryOp::Sub: return "sub";
  case BinaryOp::Mul: return "mul";
  case BinaryOp::Div: return "div";
  case BinaryOp::Mod: return "mod";
  case BinaryOp::Lt: return "lt";
  case BinaryOp::Gt: return "gt";
  case BinaryOp::Le: return "le";
  case BinaryOp::Ge: return "ge";
  case BinaryOp::Eq: return "eq";
  case BinaryOp::Ne: return "ne";
  case BinaryOp::And: return "and";
  case BinaryOp::Or: return "or";
  default: return "?";
  }
}

/**
 * @brief dump implementation
 *
 */

auto CompUnitAST::dump(int depth) const -> void {
  fmt::println("{}CompUnitAST:", indent(depth));
  if (func_def) {
    func_def->dump(depth + 1);
  }
}

auto FuncDefAST::dump(int depth) const -> void {
  fmt::println("{}FuncDefAST: {}", indent(depth), ident);
  if (func_type) {
    func_type->dump(depth + 1);
  }
  if (block) {
    block->dump(depth + 1);
  }
}

auto FuncTypeAST::dump(int depth) const -> void {
  fmt::println("{}FuncTypeAST: {}", indent(depth), type);
}

auto BlockAST::dump(int depth) const -> void {
  fmt::println("{}BlockAST:", indent(depth));
  if (stmt) {
    stmt->dump(depth + 1);
  }
}

auto StmtAST::dump(int depth) const -> void {
  fmt::println("{}StmtAST(return):", indent(depth));
  if (expr) {
    expr->dump(depth + 1);
  }
}

auto NumberAST::dump(int depth) const -> void {
  fmt::println("{}NumberAST: {}", indent(depth), val);
}

auto UnaryExprAST::dump(int depth) const -> void {
  std::string opStr = (op == UnaryOp::Neg) ? "neg" : "not";
  fmt::println("{}UnaryExprAST: {}", indent(depth), opStr);
  rhs->dump(depth + 1);
}

auto BinaryExprAST::dump(int depth) const -> void {
  fmt::println("{}BinaryExprAST: {}", indent(depth), opToString(op));
  lhs->dump(depth + 1);
  rhs->dump(depth + 1);
}

/**
 * @brief codegen implementation
 *
 */

auto CompUnitAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  if (func_def) {
    func_def->codeGen(builder);
  }
  return "";
}

auto FuncDefAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
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

auto FuncTypeAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  std::string return_type;
  if (type == "int")
    return_type = "i32 ";
  else
    return_type = "void ";
  builder.append(return_type);
  return "";
}

auto BlockAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  builder.append("{\n%entry:\n");
  if (stmt) {
    stmt->codeGen(builder);
  }
  builder.append("}\n");
  return "";
}

auto StmtAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  std::string ret_val;
  if (expr) {
    ret_val = expr->codeGen(builder);
  }
  builder.append(fmt::format("  ret {}\n", ret_val));
  return "";
}

auto NumberAST::codeGen([[maybe_unused]] ir::KoopaBuilder &builder) const
    -> std::string {
  return std::to_string(val);
}

auto UnaryExprAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
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

auto BinaryExprAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
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