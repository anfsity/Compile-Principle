/**
 * @file codegen.cpp
 * @brief Implementation of IR code generation for various AST nodes.
 *
 * This file contains the codeGen methods for all AST nodes, translating the
 * high-level syntax into Koopa IR. It utilizes a ir::KoopaBuilder to manage
 * the IR generation process, including register allocation, label management,
 * and scope control.
 *
 * ### Scoping Mechanism (`enterScope` and `exitScope`)
 *
 * Scoping is managed through the `symbol_table` within the `KoopaBuilder`.
 *
 * - `enterScope()`: When entering a function or a block, a new level is added
 *   to the symbol table stack. Any new variable or constant defined within
 *   this level will shadow definitions in outer levels.
 *
 * - `exitScope()`: When leaving a function or block, the current level is
 *   discarded. This effectively makes any local variables defined in that
 *   block inaccessible to outer blocks, maintaining the lexical scoping
 *   rules of the language. This mechanism is critical for correctly
 *   implementing nested blocks and function-local visibility.
 */

module;

#include <cassert>
#include <fmt/core.h>
#include <ranges>
#include <source_location>
#include <string>
#include <vector>

module ir.ast;

import ir_builder;
import ir.type;
import log;

using namespace ast;
using namespace detail;

/**
 * @brief Generates IR for a compilation unit (the top-level node).
 *
 * Iterates through all children (declarations and function definitions)
 * and generates IR for each.
 *
 * @param builder The IR builder context.
 * @return An empty string (top-level nodes don't return values).
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

/**
 * @brief Generates IR for function parameters.
 *
 * Checks for validity (no 'void' variables) and defines the parameter in the
 * current symbol table. If it's not a constant, it allocates space on the
 * stack.
 *
 * @param builder The IR builder context.
 * @return An empty string.
 */
auto FuncParamAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  if (btype == "void") {
    Log::panic("Semantic Error: Variable cannot be of type 'void'");
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

/**
 * @brief Generates IR for a function definition.
 *
 * This involves:
 * 1. Defining the function globally.
 * 2. Creating an entry basic block.
 * 3. Entering a new scope for the function body and parameters.
 * 4. Generating IR for parameters and the function body (block).
 * 5. Handling implicit return if the block doesn't terminate.
 * 6. Exiting the scope to clear the function's local symbols.
 *
 * @param builder The IR builder context.
 * @return An empty string.
 */
auto FuncDefAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  builder.resetCount();
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
                                  SymbolKind::Func, false);
  } else {
    builder.append(fmt::format("): {} ", btype2irType(btype)));
    builder.symtab().defineGlobal(ident, "", type::IntType::get(),
                                  SymbolKind::Func, false);
  }

  if (!block) {
    return "";
  }

  // enterScope mechanism: creates a new symbol table level for local variables
  // and parameters.
  builder.enterScope();
  builder.append(fmt::format("{{\n%entry_{}:\n", ident));
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

  // exitScope mechanism: pops the current symbol table level. All variables
  // defined within this function (including params) are removed from the lookup
  // table, preventing access from outside.
  builder.exitScope();
  builder.append("}\n");
  return "";
}

/**
 * @brief Generates IR for variable/constant declarations.
 *
 * Checks for invalid 'void' types and generates IR for each variable
 * definition.
 *
 * @param builder The IR builder context.
 * @return An empty string.
 */
auto DeclAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  if (btype == "void") {
    Log::panic("Semantic Error: Variable cannot be of type 'void'");
  }
  for (const auto &def : defs) {
    def->codeGen(builder);
  }
  return "";
}

/**
 * @brief Generates IR for a single variable definition (Def).
 *
 * Handles both global and local variables:
 * - Globals: Allocated in the global space, possibly with an initializer.
 * - Locals: Allocated on the stack using 'alloc'. Constants are tracked in the
 *   symbol table but don't result in 'alloc' instructions unless they are
 * non-const.
 *
 * @param builder The IR builder context.
 * @return An empty string.
 */
auto DefAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  if (builder.symtab().isGlobalScope()) {
    int val = 0;
    bool has_init = false;
    if (initVal) {
      val = initVal->CalcValue(builder);
      has_init = true;
    }
    if (isConst) {
      builder.symtab().defineGlobal(ident, "", type::IntType::get(),
                                    SymbolKind::Var, true, val);
    } else {
      std::string addr = builder.newVar(ident);
      if (not has_init) {
        builder.append(fmt::format("global {} = alloc i32, zeroinit\n", addr));
      } else {
        builder.append(fmt::format("global {} = alloc i32, {}\n", addr, val));
      }
      builder.symtab().defineGlobal(ident, addr, type::IntType::get(),
                                    SymbolKind::Var, false);
    }
  } else {
    // const btype var = value
    if (isConst) {
      int val = 0;
      if (initVal) {
        val = initVal->CalcValue(builder);
      }
      builder.symtab().define(ident, "", type::IntType::get(), SymbolKind::Var,
                              true, val);
    } else {
      // btype var = value
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

/**
 * @brief Generates IR for a function call.
 *
 * Lookups the function in the symbol table, generates IR for arguments,
 * and appends a 'call' instruction.
 *
 * @param builder The IR builder context.
 * @return The register name holding the return value (if any).
 */
auto FuncCallAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  auto sym = builder.symtab().lookup(ident);
  if (!sym) {
    Log::panic(fmt::format("Undefined function '{}'", ident));
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
  return ret_reg;
}

/**
 * @brief Generates IR for a block of statements (enclosed in { }).
 *
 * Handles creation and destruction of local scopes. Using `enterScope` and
 * `exitScope` ensures that variables defined within this block are not visible
 * outside.
 *
 * @param builder The IR builder context.
 * @return An empty string.
 */
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
    // exitScope mechanism: Pushes the current scope level off the stack.
    // This maintains the "stack-based" scoping where inner scopes can see
    // outer scopes but not vice-versa.
    builder.exitScope();
  }
  return "";
}

/**
 * @brief Generates IR for a return statement.
 *
 * Evaluates the return expression (if present) and appends a 'ret' instruction.
 * Marks the current basic block as closed.
 *
 * @param builder The IR builder context.
 * @return An empty string.
 */
auto ReturnStmtAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  std::string ret_val;
  if (expr) {
    ret_val = expr->codeGen(builder);
  }
  builder.setBlockClose();
  builder.append(fmt::format("  ret {}\n", ret_val));
  return "";
}

/**
 * @brief Generates IR for an assignment statement.
 *
 * Evaluates the right-hand side expression and stores the result into the
 * memory address associated with the left-hand side variable.
 *
 * @param builder The IR builder context.
 * @return An empty string.
 */
auto AssignStmtAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  std::string val_reg = expr->codeGen(builder);
  auto sym = builder.symtab().lookup(lval->ident);
  if (!sym) {
    Log::panic(
        fmt::format("Assignment to undefined variable '{}'", lval->ident));
  }
  //* lval(sym) = val_reg
  if (sym->isConst) {
    Log::panic(
        fmt::format("Error: Cannot assign to const variable '{}'", sym->name));
  }
  builder.append(fmt::format("  store {}, {}\n", val_reg, sym->irName));
  return "";
}

/**
 * @brief Generates IR for an expression statement.
 *
 * Simply evaluates the expression. The result is discarded.
 *
 * @param builder The IR builder context.
 * @return An empty string.
 */
auto ExprStmtAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  if (expr) {
    expr->codeGen(builder);
  }
  return "";
}

/**
 * @brief Generates IR for an if statement, including optional else.
 *
 * Uses basic blocks and 'br' (branching) instructions to implement the logic.
 * Handles label allocation and basic block termination with 'jump'
 * instructions.
 *
 * @param builder The IR builder context.
 * @return An empty string.
 */
auto IfStmtAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  std::string cond_reg = cond->codeGen(builder);
  int id = builder.allocLabelId();
  // clang-format off
  std::string then_label = builder.newLabel("then", id);
  std::string else_label = builder.newLabel("else", id);
  std::string end_label  = builder.newLabel("end",  id);
  // clang-format on
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

/**
 * @brief Generates IR for a while loop statement.
 *
 * Implements the loop logic using entry, body, and end basic blocks.
 * Manages the loop stack in the builder to support 'break' and 'continue'.
 *
 * @param builder The IR builder context.
 * @return An empty string.
 */
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

/**
 * @brief Generates IR for a break statement.
 *
 * Jumps to the end label of the current innermost loop.
 *
 * @param builder The IR builder context.
 * @return An empty string.
 */
auto BreakStmtAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  auto target = builder.getBreakTarget();
  builder.append(fmt::format("  jump {}\n", target));
  builder.setBlockClose();
  return "";
}

/**
 * @brief Generates IR for a continue statement.
 *
 * Jumps to the entry (condition check) label of the current innermost loop.
 *
 * @param builder The IR builder context.
 * @return An empty string.
 */
auto ContinueStmtAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  auto target = builder.getContinueTarget();
  builder.append(fmt::format("  jump {}\n", target));
  builder.setBlockClose();
  return "";
}

/**
 * @brief Generates IR for a literal number.
 *
 * Returns the string representation of the integer value.
 *
 * @param builder The IR builder context.
 * @return The string value of the number.
 */
auto NumberAST::codeGen([[maybe_unused]] ir::KoopaBuilder &builder) const
    -> std::string {
  return std::to_string(val);
}

/**
 * @brief Generates IR for an L-value (variable access).
 *
 * Loads the variable's value from its memory address into a new register.
 * If the variable is a constant, its value is returned directly.
 *
 * @param builder The IR builder context.
 * @return The register name or constant value.
 */
auto LValAST::codeGen(ir::KoopaBuilder &builder) const -> std::string {
  auto sym = builder.symtab().lookup(ident);
  if (!sym) {
    Log::panic(fmt::format("Undefined variable: '{}'", ident));
  }
  //* we can calculate const value in compile time
  if (sym->isConst) {
    return std::to_string(sym->constValue);
  }
  std::string reg = builder.newReg();
  builder.append(fmt::format("  {} = load {}\n", reg, sym->irName));
  return reg;
}

/**
 * @brief Generates IR for unary expressions.
 *
 * Performs operations like negation (sub 0, rhs) and logical NOT (eq 0, rhs).
 *
 * @param builder The IR builder context.
 * @return The register name holding the result.
 */
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
  default: Log::panic("Code Gen Error: Unknown unary op");
  }
  return ret_reg;
}

/**
 * @brief Generates IR for binary expressions.
 *
 * Includes special handling for short-circuiting logical operations (AND, OR)
 * using branching and temporary memory storage.
 *
 * @param builder The IR builder context.
 * @return The register name holding the result.
 */
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
 * @brief Evaluates a literal number at compile time.
 * @param builder The IR builder context.
 * @return The integer value of the number.
 */
auto NumberAST::CalcValue([[maybe_unused]] ir::KoopaBuilder &builder) const
    -> int {
  return val;
}

/**
 * @brief Evaluates an L-value at compile time.
 *
 * Only permitted for constant variables.
 *
 * @param builder The IR builder context.
 * @return The constant value of the variable.
 */
auto LValAST::CalcValue(ir::KoopaBuilder &builder) const -> int {
  auto sym = builder.symtab().lookup(ident);
  if (!sym) {
    Log::panic(
        fmt::format("Undefined variable '{}' in constant expression", ident));
  }
  if (!sym->isConst) {
    Log::panic(fmt::format("Variable '{}' is not a constant, cannot be used in "
                           "constant expression",
                           ident));
  }
  return sym->constValue;
}

/**
 * @brief Evaluates a unary expression at compile time.
 * @param builder The IR builder context.
 * @return The result of the unary operation.
 */
auto UnaryExprAST::CalcValue(ir::KoopaBuilder &builder) const -> int {
  int rhs_val = rhs->CalcValue(builder);
  switch (op) {
  case UnaryOp::Neg: return -rhs_val;
  case UnaryOp::Not: return !rhs_val;
  default: return 0;
  }
}

/**
 * @brief Evaluates a binary expression at compile time.
 *
 * Performs calculations for all supported binary operators.
 *
 * @param builder The IR builder context.
 * @return The result of the binary operation.
 */
auto BinaryExprAST::CalcValue(ir::KoopaBuilder &builder) const -> int {
  int rhs_val = rhs->CalcValue(builder);
  int lhs_val = lhs->CalcValue(builder);
  switch (op) {
  case BinaryOp::Add: return lhs_val + rhs_val;
  case BinaryOp::Sub: return lhs_val - rhs_val;
  case BinaryOp::Mul: return lhs_val * rhs_val;
  case BinaryOp::Div: {
    if (rhs_val == 0) {
      Log::panic("Semantic Error : Division by 0 is undefined");
    }
    return lhs_val / rhs_val;
  }
  case BinaryOp::Mod: {
    if (rhs_val == 0) {
      Log::panic("Semantic Error : Remainder by 0 is undefined");
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