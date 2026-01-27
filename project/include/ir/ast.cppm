/**
 * @file ast.cppm
 * @brief Abstract Syntax Tree (AST) definitions for the compiler.
 */

module;

#include <cassert>
#include <fmt/core.h>
#include <memory>
#include <string>
#include <vector>

export module ir.ast;

import symbol_table;
import ir_builder;

/**
 * @brief Namespace containing all AST related classes and functions.
 */
export namespace ast {

/**
 * @brief Base class for all AST nodes.
 ** Represents the "entities that make up the program".
 ** It encompasses everything, but here, it primarily serves as a global
 ** structure for things that are neither expressions nor simple statements.
 */
class BaseAST {
public:
  virtual ~BaseAST() = default;
  /**
   * @brief Dumps the AST node details for debugging.
   * @param depth The current indentation depth.
   */
  virtual auto dump(int depth) const -> void = 0;
  /**
   * @brief Generates IR code for the AST node.
   * @param builder The IR builder to use.
   * @return The IR representation of the node (usually a empty string).
   */
  virtual auto codeGen(ir::KoopaBuilder &builder) const -> std::string = 0;
};

/**
 * @brief Base class for expression AST nodes that can be evaluated.
 ** This represents "calculations that yield results".
 */
class ExprAST : public BaseAST {
public:
  /**
   * @brief Calculates the constant value of the expression if possible.
   * @param builder The IR builder context.
   * @return The calculated integer value.
   */
  virtual auto CalcValue(ir::KoopaBuilder &builder) const -> int = 0;
};

class LValAST;
class BlockAST;

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

/**
 * @brief AST node for the entire compilation unit (file).
 */
class CompUnitAST : public BaseAST {
public:
  std::vector<std::unique_ptr<BaseAST>> children;
  /**
   * @brief Constructs a compilation unit from a list of children
   * (decls/funcdefs).
   * @param _children List of top-level constructs.
   */
  CompUnitAST(std::vector<std::unique_ptr<BaseAST>> _children)
      : children(std::move(_children)) {}
  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
};

/**
 * @brief AST node for function parameters.
 */
class FuncParamAST : public BaseAST {
public:
  std::string btype;
  std::string ident;
  bool is_const;
  /**
   * @brief Constructs a function parameter.
   * @param _btype The type of the parameter (e.g., "int").
   * @param _ident The name of the parameter.
   * @param _is_const Whether the parameter is constant.
   */
  FuncParamAST(std::string _btype, std::string _ident, bool _is_const)
      : btype(std::move(_btype)), ident(std::move(_ident)),
        is_const(_is_const) {}
  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
};

/**
 * @brief AST node for function definitions.
 */
class FuncDefAST : public BaseAST {
public:
  ~FuncDefAST() override;
  std::string btype;
  std::string ident;
  std::vector<std::unique_ptr<FuncParamAST>> params;
  std::unique_ptr<BlockAST> block;
  /**
   * @brief Constructs a function definition.
   * @param _btype The return type.
   * @param _ident The function name.
   * @param _params The list of function parameters.
   * @param _block The function body.
   */
  FuncDefAST(std::string _btype, std::string _ident,
             std::vector<std::unique_ptr<FuncParamAST>> _params,
             BaseAST *_block);
  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
};

/**
 * @brief AST node for a single variable definition.
 */
class DefAST : public BaseAST {
public:
  bool is_const;
  std::string ident;
  std::unique_ptr<ExprAST> initVal;
  /**
   * @brief Constructs a variable definition.
   * @param _is_const Whether the variable is constant.
   * @param _ident The name of the variable.
   * @param _initVal The initial value expression (optional).
   */
  DefAST(bool _is_const, std::string _ident, BaseAST *_initVal)
      : is_const(_is_const), ident(std::move(_ident)) {
    if (_initVal) {
      initVal.reset(static_cast<ExprAST *>(_initVal));
    }
  }
  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
};

/**
 * @brief Base class for all statement AST nodes.
 *
 ** StmtAST indicates a side effect: the statement itself does not return a
 ** value, but it modifies the memory state or changes the control flow.
 */
class StmtAST : public BaseAST {};

/** @name Container & Bridge
 * @{
 * @brief A collection of statements (enclosed in { }), usually accompanied by
 * the opening of scope.
 */
class BlockAST : public StmtAST {
public:
  //* Semantically, it should be StmtAST, but for convenience, BaseAST is used.
  std::vector<std::unique_ptr<BaseAST>> items;
  bool createScope = true;
  /**
   * @brief Constructs a block.
   * @param _items The list of statements/declarations in the block.
   */
  BlockAST(std::vector<std::unique_ptr<BaseAST>> _items)
      : items(std::move(_items)) {}
  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
};

/**
 * @brief Expressions degenerate into statements (e.g., "x + 1;").
 */
class ExprStmtAST : public StmtAST {
public:
  // return expr
  std::unique_ptr<ExprAST> expr;
  /**
   * @brief Constructs an expression statement.
   * @param _expr The expression to evaluate.
   */
  ExprStmtAST(BaseAST *_expr) {
    if (_expr) {
      expr.reset(static_cast<ExprAST *>(_expr));
    }
  }
  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
};
/** @} */

/** @name Data Action
 * @brief AST node for assignment statements.
 * @{
 */
class AssignStmtAST : public StmtAST {
public:
  // Solved the problem of forward declaration in smart pointers
  ~AssignStmtAST() override;
  std::unique_ptr<LValAST> lval;
  std::unique_ptr<ExprAST> expr;
  /**
   * @brief Constructs an assignment statement.
   * @param _lval The left-hand side variable.
   * @param _expr The right-hand side expression.
   */
  AssignStmtAST(BaseAST *_lval, BaseAST *_expr);
  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
};

/**
 * @brief AST node for declarations (e.g., int a, b = 1;).
 */
class DeclAST : public StmtAST {
public:
  bool is_const;
  std::string btype;
  std::vector<std::unique_ptr<DefAST>> defs;
  /**
   * @brief Constructs a declaration node.
   * @param _is_const Whether the variables are constant.
   * @param _btype The type of the variables.
   * @param _defs The list of variable definitions.
   */
  DeclAST(bool _isConst, std::string _btype,
          std::vector<std::unique_ptr<DefAST>> _defs)
      : is_const(_isConst), btype(std::move(_btype)), defs(std::move(_defs)) {}
  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
};
/** @} */

/** @name Control Flow
 * @{
 * @brief AST node for return statements.
 */
class ReturnStmtAST : public StmtAST {
public:
  std::unique_ptr<ExprAST> expr;
  /**
   * @brief Constructs a return statement.
   * @param _expr The return value expression (optional).
   */
  ReturnStmtAST(BaseAST *_expr) {
    if (_expr) {
      expr.reset(static_cast<ExprAST *>(_expr));
    }
  }
  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
};

/**
 * @brief AST node for if-else statements.
 */
class IfStmtAST : public StmtAST {
public:
  std::unique_ptr<ExprAST> cond;
  std::unique_ptr<StmtAST> thenS;
  std::unique_ptr<StmtAST> elseS;
  /**
   * @brief Constructs an if statement.
   * @param _cond The condition expression.
   * @param _thenS The statement to execute if true.
   * @param _elseS The statement to execute if false (optional).
   */
  IfStmtAST(BaseAST *_cond, BaseAST *_thenS, BaseAST *_elseS) {
    cond.reset(static_cast<ExprAST *>(_cond));
    thenS.reset(static_cast<StmtAST *>(_thenS));
    elseS.reset(static_cast<StmtAST *>(_elseS));
  }
  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
};

/**
 * @brief AST node for while loop statements.
 */
class WhileStmtAST : public StmtAST {
public:
  std::unique_ptr<ExprAST> cond;
  std::unique_ptr<StmtAST> body;
  /**
   * @brief Constructs a while statement.
   * @param _cond The loop condition.
   * @param _body The loop body.
   */
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
/** @} */

/**
 * @brief AST node for literal numbers.
 */
class NumberAST : public ExprAST {
public:
  int val;
  /**
   * @brief Constructs a number node.
   * @param _val The integer value.
   */
  NumberAST(int _val) : val(_val) {};
  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
  auto CalcValue(ir::KoopaBuilder &builder) const -> int override;
};

class LValAST : public ExprAST {
public:
  std::string ident;
  /**
   * @brief Constructs an LVal node.
   * @param _ident The variable name.
   */
  LValAST(std::string _ident) : ident(std::move(_ident)) {};
  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
  auto CalcValue(ir::KoopaBuilder &builder) const -> int override;
};

/**
 * @brief AST node for function calls.
 */
class FuncCallAST : public ExprAST {
public:
  std::string ident;
  std::vector<std::unique_ptr<ExprAST>> args;
  /**
   * @brief Constructs a function call.
   * @param _ident The function name.
   * @param _args The list of expression arguments.
   */
  FuncCallAST(std::string _ident, std::vector<std::unique_ptr<ExprAST>> _args)
      : ident(std::move(_ident)), args(std::move(_args)) {}
  auto dump(int depth) const -> void override;
  auto codeGen(ir::KoopaBuilder &builder) const -> std::string override;
  auto CalcValue(ir::KoopaBuilder &builder) const -> int override;
};

class UnaryExprAST : public ExprAST {
public:
  UnaryOp op;
  std::unique_ptr<ExprAST> rhs;

  /**
   * @brief Constructs a unary expression.
   * @param _op The unary operator.
   * @param _rhs The operand expression.
   */
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

  /**
   * @brief Constructs a binary expression.
   * @param _op The binary operator.
   * @param _lhs The left operand.
   * @param _rhs The right operand.
   */
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

}; // namespace ast

/**
 * @brief Helper functions for AST operations.
 */
export namespace detail {

/**
 * @brief Helper function to generate indentation strings.
 * @param d The indentation level.
 * @return A string with (d * 2) spaces.
 */
auto inline indent(int d) -> std::string { return std::string(d * 2, ' '); }

auto inline opToString(ast::BinaryOp op) -> std::string {
  // clang-format off
  switch (op) {
  case ast::BinaryOp::Add: return "add";
  case ast::BinaryOp::Sub: return "sub";
  case ast::BinaryOp::Mul: return "mul";
  case ast::BinaryOp::Div: return "div";
  case ast::BinaryOp::Mod: return "mod";
  case ast::BinaryOp::Lt:  return "lt";
  case ast::BinaryOp::Gt:  return "gt";
  case ast::BinaryOp::Le:  return "le";
  case ast::BinaryOp::Ge:  return "ge";
  case ast::BinaryOp::Eq:  return "eq";
  case ast::BinaryOp::Ne:  return "ne";
  case ast::BinaryOp::And: return "and";
  case ast::BinaryOp::Or:  return "or";
  default: return "?";
  }
  // clang-format on
}

} // namespace detail