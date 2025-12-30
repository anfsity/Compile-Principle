// include/ast.hpp
#pragma once

#include "ir_builder.hpp"
#include "symbol_table.hpp"
#include <cassert>
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

class ExprAST : public BaseAST {
public:
  virtual auto CalcValue(ir::KoopaBuilder &builder) const -> int = 0;
};

class LValAST;
class DefAST;

//* enum class
// clang-format off
enum class BinaryOp {
    Add, Sub, Mul, Div, Mod,
    Lt, Gt, Le, Ge, Eq, Ne,
    And, Or
};

enum class UnaryOp { Neg, Not };

// clang-format on
//* enum class end

class CompUnitAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> func_def;
  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
};

class FuncDefAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> funcType;
  std::string ident;
  std::unique_ptr<BaseAST> block;
  FuncDefAST(BaseAST *_funcType, std::string _ident, BaseAST *_block)
      : funcType(_funcType), ident(std::move(_ident)), block(_block) {}

  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
};

class FuncTypeAST : public BaseAST {
public:
  // FIXME: type should use enum class
  //? or we can just use if else statement ?
  std::string type;
  FuncTypeAST(std::string _type) : type(std::move(_type)) {}
  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
};

class DeclAST : public BaseAST {
public:
  ~DeclAST() override;
  bool isConst;
  std::string btype;
  std::vector<std::unique_ptr<DefAST>> defs;
  DeclAST(bool _isConst, std::string _btype,
          std::vector<std::unique_ptr<DefAST>> *_defs);
  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
};

class DefAST : public BaseAST {
public:
  bool isConst;
  std::string ident;
  std::unique_ptr<ExprAST> initVal;
  DefAST(bool _isConst, std::string _ident, BaseAST *_initVal)
      : isConst(_isConst), ident(_ident) {
    if (_initVal) {
      initVal.reset(static_cast<ExprAST *>(_initVal));
    }
    if (ident == "void") {
      assert(false && "can't declare void type variable");
    }
  }
  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
};

//* stmt ast begin
class StmtAST : public BaseAST {};

class BlockAST : public StmtAST {
public:
  std::vector<std::unique_ptr<BaseAST>> items;
  BlockAST(std::vector<std::unique_ptr<BaseAST>> *_items) {
    if (_items) {
      items = std::move(*_items);
      delete _items;
    }
  }
  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
};

class ReturnStmtAST : public StmtAST {
public:
  // return expr
  std::unique_ptr<ExprAST> expr;
  ReturnStmtAST(BaseAST *_expr) {
    if (_expr) {
      expr.reset(static_cast<ExprAST *>(_expr));
    }
  }
  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
};

class AssignStmtAST : public StmtAST {
public:
  // Solved the problem of forward declaration in smart pointers
  ~AssignStmtAST() override;
  // ident = expr
  std::unique_ptr<LValAST> lval;
  std::unique_ptr<ExprAST> expr;
  AssignStmtAST(BaseAST *_lval, BaseAST *_expr);
  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
};

class ExprStmtAST : public StmtAST {
public:
  // return expr
  std::unique_ptr<ExprAST> expr;
  ExprStmtAST(BaseAST *_expr) {
    if (_expr) {
      expr.reset(static_cast<ExprAST *>(_expr));
    }
  }
  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
};

class IfStmtAST : public StmtAST {
public:
  std::unique_ptr<ExprAST> cond;
  std::unique_ptr<StmtAST> thenS;
  std::unique_ptr<StmtAST> elseS;
  IfStmtAST(BaseAST *_cond, BaseAST *_thenS, BaseAST *_elseS) {
    cond.reset(static_cast<ExprAST *>(_cond));
    thenS.reset(static_cast<StmtAST *>(_thenS));
    elseS.reset(static_cast<StmtAST *>(_elseS));
  }
  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
};

class WhileStmtAST : public StmtAST {
public:
  std::unique_ptr<ExprAST> cond;
  std::unique_ptr<StmtAST> body;
  WhileStmtAST(BaseAST *_cond, BaseAST *_body) {
    cond.reset(static_cast<ExprAST *>(_cond));
    body.reset(static_cast<StmtAST *>(_body));
  }
  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
};

class BreakStmtAST : public StmtAST {
public:
  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
};

class ContinueStmtAST : public StmtAST {
public:
  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
};
//* stmt ast end

//* epxr AST begin

class NumberAST : public ExprAST {
public:
  int val;
  NumberAST(int _val) : val(_val) {};
  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
  auto CalcValue(ir::KoopaBuilder &builder) const -> int override;
};

// lval value must be named value
class LValAST : public ExprAST {
public:
  std::string ident;
  LValAST(std::string _ident) : ident(std::move(_ident)) {};
  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
  auto CalcValue(ir::KoopaBuilder &builder) const -> int override;
};

class UnaryExprAST : public ExprAST {
public:
  UnaryOp op;
  std::unique_ptr<ExprAST> rhs;

  UnaryExprAST(UnaryOp _op, BaseAST *_rhs) : op(_op) {
    if (_rhs) {
      rhs.reset(static_cast<ExprAST *>(_rhs));
    }
  }

  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
  auto CalcValue(ir::KoopaBuilder &builder) const -> int override;
};

class BinaryExprAST : public ExprAST {
public:
  BinaryOp op;
  std::unique_ptr<ExprAST> lhs;
  std::unique_ptr<ExprAST> rhs;

  BinaryExprAST(BinaryOp _op, BaseAST *_lhs, BaseAST *_rhs) : op(_op) {
    if (_lhs) {
      lhs.reset(static_cast<ExprAST *>(_lhs));
    }
    if (_rhs) {
      rhs.reset(static_cast<ExprAST *>(_rhs));
    }
  }

  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
  auto CalcValue(ir::KoopaBuilder &builder) const -> int override;
};

//* expr AST end

}; // namespace ast
