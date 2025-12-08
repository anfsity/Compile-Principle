// include/ast.hpp
#pragma once

#include <fmt/core.h>
#include <memory>
#include <string>
#include "ir_builder.hpp"

namespace ast {

class BaseAST {
public:
  virtual ~BaseAST() = default;
  virtual void Dump(int depth) const = 0;
  virtual void CodeGen(ir::KoopaBuilder &builder) const = 0;
};

class CompUnitAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> func_def;
  void Dump(int depth) const override;
  void CodeGen(ir::KoopaBuilder &builder) const override;
};

class FuncDefAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> func_type;
  std::string ident;
  std::unique_ptr<BaseAST> block;
  void Dump(int depth) const override;
  void CodeGen(ir::KoopaBuilder &builder) const override;
};

class FuncTypeAST : public BaseAST {
public:
  // Fix me: type should use enum class
  std::string type;
  void Dump(int depth) const override;
  void CodeGen(ir::KoopaBuilder &builder) const override;
};

class BlockAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> stmt;
  void Dump(int depth) const override;
  void CodeGen(ir::KoopaBuilder &builder) const override;
};

class StmtAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> number;
  void Dump(int depth) const override;
  void CodeGen(ir::KoopaBuilder &builder) const override;
};

class NumberAST : public BaseAST {
public:
  int num;
  void Dump(int depth) const override;
  void CodeGen(ir::KoopaBuilder &builder) const override;
};

}; // namespace ast