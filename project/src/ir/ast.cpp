// src/ast.cpp
#include "ir/ast.hpp"
#include "Log/log.hpp"
#include "ir/ir_builder.hpp"
#include <cassert>
#include <fmt/core.h>
#include <ranges>
#include <source_location>
#include <string>
#include <vector>

using namespace ast;
using namespace detail;

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
