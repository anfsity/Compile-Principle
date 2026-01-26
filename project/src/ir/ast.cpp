/**
 * @file ast.cpp
 * @brief Implementation of Abstract Syntax Tree (AST) node methods.
 */

module;

#include <cassert>
#include <fmt/core.h>
#include <memory>
#include <string>
#include <vector>

module ir.ast;

import ir_builder;
import log;

using namespace ast;
using namespace detail;

// Destructors to solve the problem of forward declaration with smart pointers
DeclAST::~DeclAST() = default;
AssignStmtAST::~AssignStmtAST() = default;
FuncDefAST::~FuncDefAST() = default;

/**
 * @brief Constructs a FuncDefAST node.
 * @param _btype Return type of the function.
 * @param _ident Function identifier name.
 * @param _params List of function parameters.
 * @param _block Pointer to the block AST representing function body.
 */
FuncDefAST::FuncDefAST(std::string _btype, std::string _ident,
                       std::vector<std::unique_ptr<FuncParamAST>> _params,
                       BaseAST *_block)
    : btype(std::move(_btype)), ident(std::move(_ident)),
      params(std::move(_params)), block(static_cast<BlockAST *>(_block)) {}

/**
 * @brief Constructs a DeclAST node.
 * @param _isConst Whether it's a constant declaration.
 * @param _btype Base type of the declaration.
 * @param _defs List of variable definitions.
 */
DeclAST::DeclAST(bool _isConst, std::string _btype,
                 std::vector<std::unique_ptr<DefAST>> _defs)
    : isConst(_isConst), btype(std::move(_btype)), defs(std::move(_defs)) {}

/**
 * @brief Constructs an AssignStmtAST node.
 * @param _lval Left-hand side variable (LVal).
 * @param _expr Right-hand side expression.
 */
AssignStmtAST::AssignStmtAST(BaseAST *_lval, BaseAST *_expr) {
  if (_lval) {
    lval.reset(static_cast<LValAST *>(_lval));
  }
  if (_expr) {
    expr.reset(static_cast<ExprAST *>(_expr));
  }
}

/**
 * @brief Dumps CompUnitAST node details.
 * @param depth Indentation depth.
 */
auto CompUnitAST::dump(int depth) const -> void {
  fmt::println("{}CompUnitAST:", indent(depth));
  for (const auto &child : children) {
    child->dump(depth + 1);
  }
}

/**
 * @brief Dumps FuncParamAST node details.
 * @param depth Indentation depth.
 */
auto FuncParamAST::dump(int depth) const -> void {
  fmt::println("{}FuncParamAST: {} {} type: {}", indent(depth), ident,
               (isConst ? "const" : ""), btype);
}

/**
 * @brief Dumps FuncDefAST node details.
 * @param depth Indentation depth.
 */
auto FuncDefAST::dump(int depth) const -> void {
  fmt::println("{}FuncDefAST: {} type: {}", indent(depth), ident, btype);
  for (const auto &param : params) {
    param->dump(depth + 1);
  }
  if (block) {
    block->dump(depth + 1);
  }
}

/**
 * @brief Dumps DeclAST node details.
 * @param depth Indentation depth.
 */
auto DeclAST::dump(int depth) const -> void {
  std::string declType = isConst ? "ConstDecl" : "VarDecl";
  fmt::println("{}{}: {}", indent(depth), declType, btype);
  for (const auto &def : defs) {
    def->dump(depth + 1);
  }
}

/**
 * @brief Dumps DefAST node details.
 * @param depth Indentation depth.
 */
auto DefAST::dump(int depth) const -> void {
  fmt::println("{}DefAST: {}", indent(depth), ident);
  if (initVal) {
    initVal->dump(depth + 1);
  }
}

/**
 * @brief Dumps FuncCallAST node details.
 * @param depth Indentation depth.
 */
auto FuncCallAST::dump(int depth) const -> void {
  fmt::println("{}FuncCallAST: {}", indent(depth), ident);
  for (const auto &arg : args) {
    arg->dump(depth + 1);
  }
}

/**
 * @brief Dumps BlockAST node details.
 * @param depth Indentation depth.
 */
auto BlockAST::dump(int depth) const -> void {
  fmt::println("{}BlockAST:", indent(depth));
  for (const auto &item : items) {
    item->dump(depth + 1);
  }
}

/**
 * @brief Dumps ReturnStmtAST node details.
 * @param depth Indentation depth.
 */
auto ReturnStmtAST::dump(int depth) const -> void {
  fmt::println("{}ReturnStmtAST:", indent(depth));
  if (expr) {
    expr->dump(depth + 1);
  }
}

/**
 * @brief Dumps AssignStmtAST node details.
 * @param depth Indentation depth.
 */
auto AssignStmtAST::dump(int depth) const -> void {
  fmt::println("{}AssignStmtAST:", indent(depth));
  lval->dump(depth + 1);
  expr->dump(depth + 1);
}

/**
 * @brief Dumps ExprStmtAST node details.
 * @param depth Indentation depth.
 */
auto ExprStmtAST::dump(int depth) const -> void {
  fmt::println("{}ExprStmtAST:", indent(depth));
  if (expr) {
    expr->dump(depth + 1);
  }
}

/**
 * @brief Dumps IfStmtAST node details.
 * @param depth Indentation depth.
 */
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

/**
 * @brief Dumps WhileStmtAST node details.
 * @param depth Indentation depth.
 */
auto WhileStmtAST::dump(int depth) const -> void {
  fmt::println("{}WhileStmtAST:", indent(depth));
  if (cond) {
    cond->dump(depth + 1);
  }
  if (body) {
    body->dump(depth + 1);
  }
}

/**
 * @brief Dumps BreakStmtAST node details.
 * @param depth Indentation depth.
 */
auto BreakStmtAST::dump(int depth) const -> void {
  fmt::println("{}BreakAST", indent(depth));
}

/**
 * @brief Dumps ContinueStmtAST node details.
 * @param depth Indentation depth.
 */
auto ContinueStmtAST::dump(int depth) const -> void {
  fmt::println("{}ContinueAST", indent(depth));
}

/**
 * @brief Dumps NumberAST node details.
 * @param depth Indentation depth.
 */
auto NumberAST::dump(int depth) const -> void {
  fmt::println("{}NumberAST: {}", indent(depth), val);
}

/**
 * @brief Dumps LValAST node details.
 * @param depth Indentation depth.
 */
auto LValAST::dump(int depth) const -> void {
  fmt::println("{}LValAST: {}", indent(depth), ident);
}

/**
 * @brief Dumps UnaryExprAST node details.
 * @param depth Indentation depth.
 */
auto UnaryExprAST::dump(int depth) const -> void {
  std::string opStr = (op == UnaryOp::Neg) ? "neg" : "not";
  fmt::println("{}UnaryExprAST: {}", indent(depth), opStr);
  rhs->dump(depth + 1);
}

/**
 * @brief Dumps BinaryExprAST node details.
 * @param depth Indentation depth.
 */
auto BinaryExprAST::dump(int depth) const -> void {
  fmt::println("{}BinaryExprAST: {}", indent(depth), opToString(op));
  lhs->dump(depth + 1);
  rhs->dump(depth + 1);
}
