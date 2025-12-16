// src/ast.cpp
#include "ast.hpp"
#include "ir_builder.hpp"
#include <cassert>
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

DeclAST::~DeclAST() = default;
AssignStmtAST::~AssignStmtAST() = default;

DeclAST::DeclAST(bool _isConst, std::string _btype,
                 std::vector<std::unique_ptr<DefAST>> *_defs)
    : isConst(_isConst), btype(_btype) {
  if (_defs) {
    defs = std::move(*_defs);
    delete _defs;
  }
}

AssignStmtAST::AssignStmtAST(BaseAST *_lval, BaseAST *_expr) {
  if (_lval) {
    lval.reset(static_cast<LValAST *>(_lval));
  }
  if (_expr) {
    expr.reset(static_cast<ExprAST *>(_expr));
  }
}

/**
 ** @brief dump implementation
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
  if (funcType) {
    funcType->dump(depth + 1);
  }
  if (block) {
    block->dump(depth + 1);
  }
}

auto FuncTypeAST::dump(int depth) const -> void {
  fmt::println("{}FuncTypeAST: {}", indent(depth), type);
}

auto DeclAST::dump(int depth) const -> void {
  std::string declType = isConst ? "ConstDecl" : "VarDecl";
  fmt::println("{}{}: {}", indent(depth), declType, btype);
  for (const auto &def : defs) {
    def->dump(depth + 1);
  }
}

auto DefAST::dump(int depth) const -> void {
  fmt::println("{}DefAST: {}", indent(depth), ident);
  if (initVal) {
    initVal->dump(depth + 1);
  }
}

auto BlockAST::dump(int depth) const -> void {
  fmt::println("{}BlockAST:", indent(depth));
  for (const auto &item : items) {
    item->dump(depth + 1);
  }
}

auto ReturnStmtAST::dump(int depth) const -> void {
  fmt::println("{}ReturnStmtAST:", indent(depth));
  if (expr) {
    expr->dump(depth + 1);
  }
}

auto AssignStmtAST::dump(int depth) const -> void {
  fmt::println("{}AssignStmtAST:", indent(depth));
  lval->dump(depth + 1);
  expr->dump(depth + 1);
}

auto ExprStmtAST::dump(int depth) const -> void {
  fmt::println("{}ExprStmtAST:", indent(depth));
  if (expr) {
    expr->dump(depth + 1);
  }
}

auto NumberAST::dump(int depth) const -> void {
  fmt::println("{}NumberAST: {}", indent(depth), val);
}

auto LValAST::dump(int depth) const -> void {
  fmt::println("{}LValAST: {}", indent(depth), ident);
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
 ** @brief codegen implementation
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
  if (funcType) {
    funcType->codeGen(builder);
  }
  if (block) {
    builder.append("{\n%entry:\n");
    block->codeGen(builder);
    builder.append("}\n");
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

auto DeclAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  for (const auto &def : defs) {
    def->codeGen(builder);
  }
  return "";
}

auto DefAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  if (isConst) {
    int val = 0;
    if (initVal) {
      val = initVal->CalcValue(builder);
    }
    builder.getSymtable().define(ident, "", type::IntType::get(), true, val);
  } else {
    std::string addr = builder.newVar(ident);
    builder.append(fmt::format("  {} = alloc i32\n", addr));
    builder.getSymtable().define(ident, addr, type::IntType::get(), false);
    if (initVal) {
      std::string val_reg = initVal->codeGen(builder);
      builder.append(fmt::format("  store {}, {}\n", val_reg, addr));
    }
  }
  return "";
}

auto BlockAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  builder.enterScope();
  for (const auto &item : items) {
    item->codeGen(builder);
  }
  builder.exitScope();
  return "";
}

auto ReturnStmtAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  std::string ret_val;
  if (expr) {
    ret_val = expr->codeGen(builder);
  }
  builder.append(fmt::format("  ret {}\n", ret_val));
  return "";
}

auto AssignStmtAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  std::string val_reg = expr->codeGen(builder);
  auto sym = builder.getSymtable().lookup(lval->ident);
  assert(sym && "Assignment to undefined variable");
  if (sym->isConst) {
    fmt::println(stderr, "Error: Cannot assign to const variable '{}'",
                 sym->name);
    std::abort();
  }
  builder.append(fmt::format("  store {}, {}\n", val_reg, sym->irName));
  return "";
}

auto ExprStmtAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  if (expr) {
    expr->codeGen(builder);
  }
  return "";
}

auto NumberAST::codeGen([[maybe_unused]] ir::KoopaBuilder &builder) const
    -> std::string {
  return std::to_string(val);
}

auto LValAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  auto sym = builder.getSymtable().lookup(ident);
  assert(sym && "Undefined variable");
  if (sym->isConst) {
    return std::to_string(sym->constValue);
  }
  std::string reg = builder.newReg();
  builder.append(fmt::format("  {} = load {}\n", reg, sym->irName));
  return reg;
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
    builder.append(fmt::format("  {} = ne {}, 0\n", rhs_tmp, rhs_reg));
    lhs_reg = lhs_tmp;
    rhs_reg = rhs_tmp;
  }
  builder.append(
      fmt::format("  {} = {} {}, {}\n", ret_reg, ir_op, lhs_reg, rhs_reg));
  return ret_reg;
}

/**
 ** @brief calculate constexpr value
 *
 */

auto NumberAST::CalcValue([[maybe_unused]] ir::KoopaBuilder &builder) const
    -> int {
  return val;
}

auto LValAST::CalcValue(ir::KoopaBuilder &builder) const -> int {
  auto sym = builder.getSymtable().lookup(ident);
  assert(sym && "Undefined variable in constant expression");
  if (!sym->isConst) {
    throw std::runtime_error("Semantic Error: Variable '" + ident +
                             "' is not a constant.");
  }
  return sym->constValue;
}

auto UnaryExprAST::CalcValue(ir::KoopaBuilder &builder) const -> int {
  int rhs_val = rhs->CalcValue(builder);
  switch (op) {
  case UnaryOp::Neg: return -rhs_val;
  case UnaryOp::Not: return !rhs_val;
  default: return 0;
  }
}

auto BinaryExprAST::CalcValue(ir::KoopaBuilder &builder) const -> int {
  int rhs_val = rhs->CalcValue(builder);
  int lhs_val = lhs->CalcValue(builder);
  switch (op) {
  case BinaryOp::Add: return lhs_val + rhs_val;
  case BinaryOp::Sub: return lhs_val - rhs_val;
  case BinaryOp::Mul: return lhs_val * rhs_val;
  case BinaryOp::Div: {
    if (rhs_val == 0) {
      throw std::runtime_error("Semantic Error : Division by 0 is undefined");
    }
    return lhs_val / rhs_val;
  }
  case BinaryOp::Mod: {
    if (rhs_val == 0) {
      throw std::runtime_error("Semantic Error : Remainder by 0 is undefined");
    }
    return lhs_val % rhs_val;
  }
  case BinaryOp::Lt: return lhs_val < rhs_val;
  case BinaryOp::Gt: return lhs_val > rhs_val;
  case BinaryOp::Le: return lhs_val <= rhs_val;
  case BinaryOp::Ge: return lhs_val >= rhs_val;
  case BinaryOp::Eq: return lhs_val == rhs_val;
  case BinaryOp::Ne: return lhs_val != rhs_val;
  case BinaryOp::And: return lhs_val && rhs_val;
  case BinaryOp::Or: return lhs_val || rhs_val;
  default: return 0;
  }
}