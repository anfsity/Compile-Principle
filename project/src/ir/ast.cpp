// src/ast.cpp
#include "ir/ast.hpp"
#include "ir/ir_builder.hpp"
#include <cassert>
#include <fmt/core.h>
#include <ranges>
#include <source_location>
#include <string>
#include <vector>

using namespace ast;
using namespace detail;

auto indent(int d) -> std::string { return std::string(d * 2, ' '); }

auto opToString(BinaryOp op) -> std::string {
  // clang-format off
  switch (op) {
  case BinaryOp::Add: return "add";
  case BinaryOp::Sub: return "sub";
  case BinaryOp::Mul: return "mul";
  case BinaryOp::Div: return "div";
  case BinaryOp::Mod: return "mod";
  case BinaryOp::Lt:  return "lt";
  case BinaryOp::Gt:  return "gt";
  case BinaryOp::Le:  return "le";
  case BinaryOp::Ge:  return "ge";
  case BinaryOp::Eq:  return "eq";
  case BinaryOp::Ne:  return "ne";
  case BinaryOp::And: return "and";
  case BinaryOp::Or:  return "or";
  default: return "?";
  }
  // clang-format on
}

// auto panic(std::string_view msg,
//            const std::source_location &loc = std::source_location::current()) {
//   throw std::runtime_error(fmt::format("[Error]: {}\n  at {}:{}:{}\n",
//                                        msg,
//                                        loc.file_name(),
//                                        loc.line(),
//                                        loc.function_name()));
// }

// solve the problem forward declaration
DeclAST::~DeclAST() = default;
AssignStmtAST::~AssignStmtAST() = default;
FuncDefAST::~FuncDefAST() = default;

FuncDefAST::FuncDefAST(std::string _btype, std::string _ident,
                       std::vector<std::unique_ptr<FuncParamAST>> _params,
                       BaseAST *_block)
    : btype(std::move(_btype)), ident(std::move(_ident)),
      params(std::move(_params)), block(static_cast<BlockAST *>(_block)) {}

DeclAST::DeclAST(bool _isConst, std::string _btype,
                 std::vector<std::unique_ptr<DefAST>> _defs)
    : isConst(_isConst), btype(std::move(_btype)), defs(std::move(_defs)) {}

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
  for (const auto &child : children) {
    child->dump(depth + 1);
  }
}

auto FuncParamAST::dump(int depth) const -> void {
  fmt::println("{}FuncParamAST: {} {} type: {}", indent(depth), ident,
               (isConst ? "const" : ""), btype);
}

auto FuncDefAST::dump(int depth) const -> void {
  fmt::println("{}FuncDefAST: {} type: {}", indent(depth), ident, btype);
  for (const auto &param : params) {
    param->dump(depth + 1);
  }
  if (block) {
    block->dump(depth + 1);
  }
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

auto FuncCallAST::dump(int depth) const -> void {
  fmt::println("{}FuncCallAST: {}", indent(depth), ident);
  for (const auto &arg : args) {
    arg->dump(depth + 1);
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

auto IfStmtAST::dump(int depth) const -> void {
  fmt::println("{}IfStmtAST:", indent(depth));
  if (cond) {
    cond->dump(depth + 1);
  }
  if (thenS) {
    thenS->dump(depth + 1);
  }
  if (elseS) {
    elseS->dump(depth + 1);
  }
}

auto WhileStmtAST::dump(int depth) const -> void {
  fmt::println("{}WhileStmtAST:", indent(depth));
  if (cond) {
    cond->dump(depth + 1);
  }
  if (body) {
    body->dump(depth + 1);
  }
}

auto BreakStmtAST::dump(int depth) const -> void {
  fmt::println("{}BreakAST", indent(depth));
}

auto ContinueStmtAST::dump(int depth) const -> void {
  fmt::println("{}ContinueAST", indent(depth));
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
  for (const auto &child : children) {
    child->codeGen(builder);
    if (&child != &children.back()) {
      builder.append("\n");
    }
  }
  return "";
}

auto FuncParamAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  if (btype == "void") {
    throw std::runtime_error(
        "Semantic Error: Variable cannot be of type 'void'");
  }
  if (isConst) {
    builder.symtab().define(ident, "", type::IntType::get(), SymbolKind::Var,
                            true, 0);
  } else {
    std::string addr = builder.newReg();
    builder.append(fmt::format("  {} = alloc i32\n", addr));
    builder.symtab().define(ident, addr, type::IntType::get(), SymbolKind::Var,
                            false);
    builder.append(fmt::format("  store @{}, {}\n", ident, addr));
  }
  return "";
}

auto FuncDefAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  auto btype2irType = [](std::string_view s) -> std::string {
    return (s == "int" ? "i32" : "");
  };
  builder.append(fmt::format("{} @", (block ? "fun" : "decl")));
  builder.append(ident);
  builder.append("(");
  for (const auto &param : params) {
    builder.append(
        fmt::format("@{}: {}", param->ident, btype2irType(param->btype)));
    if (&param != &params.back()) {
      builder.append(", ");
    }
  }

  if (btype == "void") {
    builder.append(") ");
    builder.symtab().defineGlobal(ident, "", type::VoidType::get(),
                                  detail::SymbolKind::Func, false);
  } else {
    builder.append(fmt::format("): {} ", btype2irType(btype)));
    builder.symtab().defineGlobal(ident, "", type::IntType::get(),
                                  detail::SymbolKind::Func, false);
  }

  if (!block) {
    return "";
  }

  builder.enterScope();
  builder.append("{\n%entry:\n");
  for (const auto &param : params) {
    param->codeGen(builder);
  }

  builder.clearBlockClose();
  block->createScope = false;
  block->codeGen(builder);

  // e.g. int main() { int a; }
  // there is no return value.
  if (!builder.isBlockClose()) {
    if (btype == "void") {
      builder.append("  ret\n");
    } else {
      builder.append("  ret 0\n");
    }
    builder.setBlockClose();
  }

  builder.exitScope();
  builder.append("}\n");
  return "";
}

auto DeclAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  if (btype == "void") {
    throw std::runtime_error(
        "Semantic Error: Variable cannot be of type 'void'");
  }
  for (const auto &def : defs) {
    def->codeGen(builder);
  }
  return "";
}

auto DefAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  if (builder.symtab().isGlobalScope()) {
    int val = 0;
    if (initVal) {
      val = initVal->CalcValue(builder);
    }
    if (isConst) {
      builder.symtab().defineGlobal(ident, "", type::IntType::get(),
                                    detail::SymbolKind::Var, true, val);
    } else {
      std::string addr = builder.newVar(ident);
      builder.append(fmt::format("global {} = alloc i32, {}\n", addr, val));
      builder.symtab().defineGlobal(ident, addr, type::IntType::get(),
                                    detail::SymbolKind::Var, false);
    }
  } else {
    //* const btype var = value
    if (isConst) {
      int val = 0;
      if (initVal) {
        val = initVal->CalcValue(builder);
      }
      builder.symtab().define(ident, "", type::IntType::get(), SymbolKind::Var,
                              true, val);
    } else {
      //* btype var = value
      std::string addr = builder.newVar(ident);
      builder.append(fmt::format("  {} = alloc i32\n", addr));
      builder.symtab().define(ident, addr, type::IntType::get(),
                              SymbolKind::Var, false);
      if (initVal) {
        std::string val_reg = initVal->codeGen(builder);
        builder.append(fmt::format("  store {}, {}\n", val_reg, addr));
      }
    }
  }
  return "";
}

auto FuncCallAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  auto sym = builder.symtab().lookup(ident);
  if (!sym) {
    throw std::runtime_error(fmt::format("Undefined function '{}'", ident));
  }

  std::vector<std::string> arg_val;
  for (const auto &arg : args) {
    arg_val.emplace_back(arg->codeGen(builder));
  }
  std::string ret_reg;
  if (sym->type->is_void()) {
    builder.append(fmt::format("  call @{}(", ident));
  } else {
    ret_reg = builder.newReg();
    builder.append(fmt::format("  {} = call @{}(", ret_reg, ident));
  }
  for (const auto &val : arg_val) {
    builder.append(val);
    if (&val != &arg_val.back()) {
      builder.append(", ");
    }
  }
  builder.append(")\n");
  // builder.append(fmt::format("//! [Debug]: args size: {}\n", args.size()));
  return ret_reg;
}

auto BlockAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  if (this->createScope) {
    builder.enterScope();
  }
  for (const auto &item : items) {
    if (builder.isBlockClose()) {
      continue;
    }
    item->codeGen(builder);
  }
  if (this->createScope) {
    builder.exitScope();
  }
  return "";
}

//* return expr
auto ReturnStmtAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  std::string ret_val;
  if (expr) {
    ret_val = expr->codeGen(builder);
  }
  builder.setBlockClose();
  builder.append(fmt::format("  ret {}\n", ret_val));
  return "";
}

auto AssignStmtAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  std::string val_reg = expr->codeGen(builder);
  auto sym = builder.symtab().lookup(lval->ident);
  if (!sym) {
    throw std::runtime_error(
        fmt::format("Assignment to undefined variable '{}'", lval->ident));
  }
  //* lval(sym) = val_reg
  if (sym->isConst) {
    throw std::runtime_error(
        fmt::format("Error: Cannot assign to const variable '{}'", sym->name));
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

auto IfStmtAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  std::string cond_reg = cond->codeGen(builder);
  int id = builder.allocLabelId();
  std::string then_label = builder.newLabel("then", id);
  std::string else_label = builder.newLabel("else", id);
  std::string end_label = builder.newLabel("end", id);
  if (elseS) {
    builder.append(
        fmt::format("  br {}, {}, {}\n", cond_reg, then_label, else_label));
  } else {
    builder.append(
        fmt::format("  br {}, {}, {}\n", cond_reg, then_label, end_label));
  }

  // not jump
  builder.append(fmt::format("{}:\n", then_label));
  builder.clearBlockClose();
  thenS->codeGen(builder);
  if (!builder.isBlockClose()) {
    builder.append(fmt::format("  jump {}\n", end_label));
  }

  // jump
  if (elseS) {
    builder.append(fmt::format("{}:\n", else_label));
    builder.clearBlockClose();
    elseS->codeGen(builder);
    if (!builder.isBlockClose()) {
      builder.append(fmt::format("  jump {}\n", end_label));
    }
  }
  builder.append(fmt::format("{}:\n", end_label));
  // every basic block (entry) need to pair a block close.
  builder.clearBlockClose();
  return "";
}

auto WhileStmtAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  int id = builder.allocLabelId();
  std::string entry_label = builder.newLabel("while_entry", id);
  std::string body_label = builder.newLabel("while_body", id);
  std::string end_label = builder.newLabel("while_end", id);
  builder.pushLoop(entry_label, end_label);
  builder.append(fmt::format("  jump {}\n", entry_label));

  builder.append(fmt::format("{}:\n", entry_label));
  std::string cond_reg = cond->codeGen(builder);
  builder.append(
      fmt::format("  br {}, {}, {}\n", cond_reg, body_label, end_label));

  builder.append(fmt::format("{}:\n", body_label));
  if (body) {
    builder.clearBlockClose();
    body->codeGen(builder);
  }
  if (!builder.isBlockClose()) {
    builder.append(fmt::format("  jump {}\n", entry_label));
  }

  builder.append(fmt::format("{}:\n", end_label));
  builder.popLoop();
  builder.clearBlockClose();
  return "";
}

auto BreakStmtAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  auto target = builder.getBreakTarget();
  builder.append(fmt::format("  jump {}\n", target));
  builder.setBlockClose();
  return "";
}

auto ContinueStmtAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  auto target = builder.getContinueTarget();
  builder.append(fmt::format("  jump {}\n", target));
  builder.setBlockClose();
  return "";
}

auto NumberAST::codeGen([[maybe_unused]] ir::KoopaBuilder &builder) const
    -> std::string {
  return std::to_string(val);
}

auto LValAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  auto sym = builder.symtab().lookup(ident);
  if (!sym) {
    throw std::runtime_error(fmt::format("Undefined variable: '{}'", ident));
  }
  //* we can calculate const value in compile time
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
  if (op == BinaryOp::And) {
    std::string tmp_addr = builder.newVar("and_res");
    builder.append(fmt::format("  {} = alloc i32\n", tmp_addr));

    std::string lhs_reg = lhs->codeGen(builder);
    int id = builder.allocLabelId();
    std::string true_label = builder.newLabel("and_true", id);
    std::string false_label = builder.newLabel("and_false", id);
    std::string end_label = builder.newLabel("and_end", id);

    std::string lhs_bool = builder.newReg();
    builder.append(fmt::format("  {} = ne {}, 0\n", lhs_bool, lhs_reg));
    builder.append(
        fmt::format("  br {}, {}, {}\n", lhs_bool, true_label, false_label));

    // true branch
    builder.append(fmt::format("{}:\n", true_label));
    std::string rhs_reg = rhs->codeGen(builder);
    std::string rhs_bool = builder.newReg();
    builder.append(fmt::format("  {} = ne {}, 0\n", rhs_bool, rhs_reg));
    builder.append(fmt::format("   store {}, {}\n", rhs_bool, tmp_addr));
    builder.append(fmt::format("  jump {}\n", end_label));

    // false branch
    builder.append(fmt::format("{}:\n", false_label));
    builder.append(fmt::format("   store 0, {}\n", tmp_addr));
    builder.append(fmt::format("  jump {}\n", end_label));

    // end
    builder.append(fmt::format("{}:\n", end_label));
    std::string ret_reg = builder.newReg();
    builder.append(fmt::format("  {} = load {}\n", ret_reg, tmp_addr));

    return ret_reg;
  }
  if (op == BinaryOp::Or) {
    std::string tmp_addr = builder.newVar("or_res");
    builder.append(fmt::format("  {} = alloc i32\n", tmp_addr));

    std::string lhs_reg = lhs->codeGen(builder);

    int id = builder.allocLabelId();
    std::string true_label = builder.newLabel("or_true", id);
    std::string false_label = builder.newLabel("or_false", id);
    std::string end_label = builder.newLabel("or_end", id);

    std::string lhs_bool = builder.newReg();
    builder.append(fmt::format("  {} = ne {}, 0\n", lhs_bool, lhs_reg));
    builder.append(
        fmt::format("  br {}, {}, {}\n", lhs_bool, true_label, false_label));

    // true branch
    builder.append(fmt::format("{}:\n", true_label));
    builder.append(fmt::format("  store 1, {}\n", tmp_addr));
    builder.append(fmt::format("  jump {}\n", end_label));

    // false branch
    builder.append(fmt::format("{}:\n", false_label));
    std::string rhs_reg = rhs->codeGen(builder);
    std::string rhs_bool = builder.newReg();
    builder.append(fmt::format("  {} = ne {}, 0\n", rhs_bool, rhs_reg));
    builder.append(fmt::format("   store {}, {}\n", rhs_bool, tmp_addr));
    builder.append(fmt::format("  jump {}\n", end_label));

    // end
    builder.append(fmt::format("{}:\n", end_label));
    std::string ret_reg = builder.newReg();
    builder.append(fmt::format("  {} = load {}\n", ret_reg, tmp_addr));

    return ret_reg;
  }
  // remain binary operator
  std::string lhs_reg = lhs->codeGen(builder);
  std::string rhs_reg = rhs->codeGen(builder);
  std::string ret_reg = builder.newReg();
  std::string ir_op = opToString(op);
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
  auto sym = builder.symtab().lookup(ident);
  if (!sym) {
    throw std::runtime_error(
        fmt::format("Undefined variable '{}' in constant expression", ident));
  }
  if (!sym->isConst) {
    throw std::runtime_error(
        fmt::format("Variable '{}' is not a constant, cannot be used in "
                    "constant expression",
                    ident));
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