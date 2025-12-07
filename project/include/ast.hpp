// include/ast.hpp
#pragma once

#include <fmt/core.h>
#include <memory>
#include <string>

namespace ast {

class BaseAST {
public:
  virtual ~BaseAST() = default;
  virtual void Dump(int depth) const = 0;
};

class CompUnitAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> func_def;
  void Dump(int depth) const override;
};

class FuncDefAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> func_type;
  std::string ident;
  std::unique_ptr<BaseAST> block;
  void Dump(int depth) const override;
};

class FuncTypeAST : public BaseAST {
public:
  std::string type;
  void Dump(int depth) const override;
};

class BlockAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> stmt;
  void Dump(int depth) const override;
};

class StmtAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> number;
  void Dump(int depth) const override;
};

class NumberAST : public BaseAST {
public:
  int num;
  void Dump(int depth) const override;
};

}; // namespace ast