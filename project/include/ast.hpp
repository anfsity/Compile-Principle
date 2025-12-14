// include/ast.hpp
#pragma once

#include "ir_builder.hpp"
#include <fmt/core.h>
#include <memory>
#include <string>

namespace ast {

// ast interface
class BaseAST {
public:
  virtual ~BaseAST() = default;
  virtual auto dump(int depth) const -> void = 0;
  virtual auto codeGen(ir::KoopaBuilder &builder) const -> std::string = 0;
};

class ExprAST : public BaseAST {};

// enum class
// clang-format off
enum class BinaryOp {
    Add, Sub, Mul, Div, Mod,
    Lt, Gt, Le, Ge, Eq, Ne,
    And, Or
};

enum class UnaryOp { Neg, Not };

// clang-format on
// enum class end

class CompUnitAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> func_def;
  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
};

class FuncDefAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> func_type;
  std::string ident;
  std::unique_ptr<BaseAST> block;
  FuncDefAST(std::unique_ptr<BaseAST> _func_type, std::string _ident,
             std::unique_ptr<BaseAST> _block)
      : func_type(std::move(_func_type)), ident(_ident),
        block(std::move(_block)) {}
  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
};

class FuncTypeAST : public BaseAST {
public:
  // FIXME: type should use enum class
  std::string type;
  FuncTypeAST(std::string _type) : type(_type) {}
  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
};

class BlockAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> stmt;
  BlockAST(std::unique_ptr<BaseAST> _stmt) : stmt(std::move(_stmt)) {}
  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
};

class StmtAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> expr;
  StmtAST(std::unique_ptr<BaseAST> _expr) : expr(std::move(_expr)) {}
  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
};

// epxr AST begin

class NumberAST : public ExprAST {
public:
  int val;
  NumberAST(int _val) : val(_val) {};
  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
};

class UnaryExprAST : public ExprAST {
public:
  UnaryOp op;
  std::unique_ptr<BaseAST> rhs;

  UnaryExprAST(UnaryOp _op, std::unique_ptr<BaseAST> _rhs)
      : op(_op), rhs(std::move(_rhs)) {}

  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
};

class BinaryExprAST : public ExprAST {
public:
  BinaryOp op;
  std::unique_ptr<BaseAST> lhs;
  std::unique_ptr<BaseAST> rhs;

  BinaryExprAST(BinaryOp _op, std::unique_ptr<BaseAST> _lhs,
                std::unique_ptr<BaseAST> _rhs)
      : op(_op), lhs(std::move(_lhs)), rhs(std::move(_rhs)) {}

  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
};

// expr AST end

}; // namespace ast
