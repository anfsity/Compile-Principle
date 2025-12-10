// include/ast.hpp
#pragma once

#include "ir_builder.hpp"
#include <fmt/core.h>
#include <memory>
#include <string>

namespace ast {

class BaseAST {
public:
  virtual ~BaseAST() = default;
  virtual void dump(int depth) const = 0;
  virtual std::string codeGen(ir::KoopaBuilder &builder) const = 0;
};

class ExprAST : public BaseAST {};

enum class BinaryOp {
    Add, Sub, Mul, Div, Mod,
    Lt, Gt, Le, Ge, Eq, Ne,
    And, Or
};

enum class UnaryOp { Neg, Not };

class CompUnitAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> func_def;
  void dump(int depth) const override;
  std::string codeGen(ir::KoopaBuilder &builder) const override;
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
  void dump(int depth) const override;
  std::string codeGen(ir::KoopaBuilder &builder) const override;
};

class FuncTypeAST : public BaseAST {
public:
  // FIXME: type should use enum class
  std::string type;
  FuncTypeAST(std::string _type) : type(_type) {}
  void dump(int depth) const override;
  std::string codeGen(ir::KoopaBuilder &builder) const override;
};

class BlockAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> stmt;
  BlockAST(std::unique_ptr<BaseAST> _stmt) : stmt(std::move(_stmt)) {}
  void dump(int depth) const override;
  std::string codeGen(ir::KoopaBuilder &builder) const override;
};

class StmtAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> expr;
  StmtAST(std::unique_ptr<BaseAST> _expr) : expr(std::move(_expr)) {}
  void dump(int depth) const override;
  std::string codeGen(ir::KoopaBuilder &builder) const override;
};

class NumberAST : public ExprAST {
public:
  int val;
  NumberAST(int _val) : val(_val) {};
  void dump(int depth) const override;
  std::string codeGen(ir::KoopaBuilder &builder) const override;
};

class UnaryExprAST : public ExprAST {
public:
  UnaryOp op;
  std::unique_ptr<BaseAST> rhs;

  UnaryExprAST(UnaryOp _op, std::unique_ptr<BaseAST> _rhs)
      : op(_op), rhs(std::move(_rhs)) {}

  void dump(int depth) const override;
  std::string codeGen(ir::KoopaBuilder &builder) const override;
};

class BinaryExprAST : public ExprAST {
public:
  BinaryOp op;
  std::unique_ptr<BaseAST> lhs;
  std::unique_ptr<BaseAST> rhs;

  BinaryExprAST(BinaryOp _op, std::unique_ptr<BaseAST> _lhs,
                std::unique_ptr<BaseAST> _rhs)
      : op(_op), lhs(std::move(_lhs)), rhs(std::move(_rhs)) {}

  void dump(int depth) const override;
  std::string codeGen(ir::KoopaBuilder &builder) const override;
};

}; // namespace ast
