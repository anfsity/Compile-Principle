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
  virtual void codeGen(ir::KoopaBuilder &builder) const = 0;
};

class CompUnitAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> func_def;
  void dump(int depth) const override;
  void codeGen(ir::KoopaBuilder &builder) const override;
};

class FuncDefAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> func_type;
  std::string ident;
  std::unique_ptr<BaseAST> block;
  void dump(int depth) const override;
  void codeGen(ir::KoopaBuilder &builder) const override;
};

class FuncTypeAST : public BaseAST {
public:
  // Fix me: type should use enum class
  std::string type;
  void dump(int depth) const override;
  void codeGen(ir::KoopaBuilder &builder) const override;
};

class BlockAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> stmt;
  void dump(int depth) const override;
  void codeGen(ir::KoopaBuilder &builder) const override;
};

class StmtAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> number;
  void dump(int depth) const override;
  void codeGen(ir::KoopaBuilder &builder) const override;
};

class NumberAST : public BaseAST {
public:
  int num;
  void dump(int depth) const override;
  void codeGen(ir::KoopaBuilder &builder) const override;
};

}; // namespace ast