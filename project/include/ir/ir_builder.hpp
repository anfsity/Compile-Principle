/**
 * @file ir_builder.hpp
 * @brief Definition of the KoopaBuilder class for IR generation.
 *
 * This file providing the infrastructure for building Koopa IR text.
 * It manages symbol tables, register numbering, and label generation.
 */
#pragma once

#include "Log/log.hpp"
#include "symbol_table.hpp"
#include <fmt/core.h>
#include <string>

namespace ir {

/**
 * @brief Context for loop control flow (break/continue).
 */
struct LoopContext {
  std::string continueTarget; ///< Label to jump for 'continue'
  std::string breakTarget;    ///< Label to jump for 'break'
};

/**
 * @brief Helper class to construct Koopa IR text strings.
 */
class KoopaBuilder {
private:
  // clang-format off
  std::string buffer;            ///< The IR text buffer.
  int count_reg = 0;             ///< Counter for local virtual registers (%0, %1, ...).
  int count_name = 0;            ///< Counter for uniquely naming local variables.
  int count_label = 0;           ///< Counter for basic block labels.
  bool _is_block_closed = false; ///< Flag to track if the current basic block already has a terminator.
  std::vector<LoopContext> loopstk; ///< Stack of loop contexts for nested loops.
  SymbolTable _symtab;           ///< Associated symbol table for semantic analysis and IR mapping.
  // clang-format on

public:
  /**
   * @brief Initializes the builder and declares built-in SysY library
   * functions.
   */
  KoopaBuilder() {
    // register library funtions
    [&]() -> void {
      buffer.append("decl @getint(): i32\n");
      buffer.append("decl @getch(): i32\n");
      buffer.append("decl @getarray(*i32): i32\n");
      buffer.append("decl @putint(i32)\n");
      buffer.append("decl @putch(i32)\n");
      buffer.append("decl @putarray(i32, *i32)\n");
      buffer.append("decl @starttime()\n");
      buffer.append("decl @stoptime()\n\n");

      _symtab.defineGlobal("getint", "", type::IntType::get(),
                           detail::SymbolKind::Func, false);
      _symtab.defineGlobal("getch", "", type::IntType::get(),
                           detail::SymbolKind::Func, false);
      _symtab.defineGlobal("getarray", "", type::IntType::get(),
                           detail::SymbolKind::Func, false);
      _symtab.defineGlobal("putint", "", type::VoidType::get(),
                           detail::SymbolKind::Func, false);
      _symtab.defineGlobal("putch", "", type::VoidType::get(),
                           detail::SymbolKind::Func, false);
      _symtab.defineGlobal("putarray", "", type::VoidType::get(),
                           detail::SymbolKind::Func, false);
      _symtab.defineGlobal("starttime", "", type::VoidType::get(),
                           detail::SymbolKind::Func, false);
      _symtab.defineGlobal("stoptime", "", type::VoidType::get(),
                           detail::SymbolKind::Func, false);
    }();
  }

  /**
   * @brief Directly appends a string to the IR buffer.
   */
  auto append(std::string_view str) -> void { buffer += str; }

  /**
   * @brief Generates a new unique virtual register name.
   * @return std::string e.g., "%12"
   */
  auto newReg() -> std::string { return fmt::format("%{}", count_reg++); }

  /**
   * @brief Generates a new unique local variable name.
   * @return std::string e.g., "@ident_5"
   */
  auto newVar(std::string_view ident) -> std::string {
    return fmt::format("@{}_{}", ident, count_name++);
  };

  /**
   * @brief Allocates a new unique label ID.
   */
  auto allocLabelId() -> int { return count_label++; }

  /**
   * @brief Constructs a label name from a prefix and ID.
   */
  auto newLabel(std::string_view prefix, int id) -> std::string {
    return fmt::format("%{}_{}", prefix, id);
  }

  /**
   * @brief Allocates and constructs a unique label name.
   */
  auto allocUniqueLabel(std::string_view prefix) -> std::string {
    return newLabel(prefix, allocLabelId());
  }

  /** @name Block Termination Management
   *  @{
   */
  auto isBlockClose() -> bool { return _is_block_closed; }
  auto setBlockClose() -> void { _is_block_closed = true; }
  auto clearBlockClose() -> void { _is_block_closed = false; }
  /** @} */

  /** @name Loop Context Management
   *  @{
   */
  auto pushLoop(const std::string &cTar, const std::string &bTar) -> void {
    loopstk.emplace_back(cTar, bTar);
  }
  auto popLoop() -> void { loopstk.pop_back(); }
  auto getBreakTarget() -> std::string_view {
    if (loopstk.empty()) {
      Log::panic("Semantic Error: 'break' statement not within loop.");
    }
    return loopstk.back().breakTarget;
  }
  auto getContinueTarget() -> std::string_view {
    if (loopstk.empty()) {
      Log::panic("Semantic Error: 'continue' statement not within loop.");
    }
    return loopstk.back().continueTarget;
  }
  /** @} */

  /**
   * @brief Resets all counters and flags (used when starting a new function).
   */
  auto resetCount() -> void {
    count_reg = 0;
    count_name = 0;
    count_label = 0;
    _is_block_closed = false;
  }

  /** @name Symbol Table Proxy
   *  @{
   */
  auto symtab() -> SymbolTable & { return _symtab; }
  auto enterScope() -> void { _symtab.enterScope(); }
  auto exitScope() -> void { _symtab.exitScope(); }
  /** @} */

  /**
   * @brief Finalizes the construction and retrieves the generated string.
   *
   * This method performs a destructive move: it transfers the ownership of
   * the internal buffer to the caller. After this call, the builder will
   * be in an empty state.
   *
   * @note The return value must be used, otherwise the data is lost.
   *
   * @return std::string The generated content.
   */

  [[nodiscard]] auto build() -> std::string { return std::move(buffer); }
};
} // namespace ir