// include/ir_builder.hpp
#pragma once

#include "Log/log.hpp"
#include "symbol_table.hpp"
#include <fmt/core.h>
#include <string>

namespace ir {

struct LoopContext {
  std::string continueTarget;
  std::string breakTarget;
};

class KoopaBuilder {
private:
  std::string buffer;
  int count_reg = 0;
  int count_name = 0;
  int count_label = 0;
  bool _is_block_closed = false;
  std::vector<LoopContext> loopstk;
  SymbolTable _symtab;

public:
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

  auto append(std::string_view str) -> void { buffer += str; }
  // registers allocate
  auto newReg() -> std::string { return fmt::format("%{}", count_reg++); }
  auto newVar(std::string_view ident) -> std::string {
    return fmt::format("@{}_{}", ident, count_name++);
  };

  // if while allocate
  auto allocLabelId() -> int { return count_label++; }
  auto newLabel(std::string_view prefix, int id) -> std::string {
    return fmt::format("%{}_{}", prefix, id);
  }
  auto allocUniqueLabel(std::string_view prefix) -> std::string {
    return newLabel(prefix, allocLabelId());
  }

  // solve multiple return statements
  auto isBlockClose() -> bool { return _is_block_closed; }
  auto setBlockClose() -> void { _is_block_closed = true; }
  auto clearBlockClose() -> void { _is_block_closed = false; }

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

  auto resetCount() -> void {
    count_reg = 0;
    count_name = 0;
    count_label = 0;
    _is_block_closed = false;
  }

  auto symtab() -> SymbolTable & { return _symtab; }
  auto enterScope() -> void { _symtab.enterScope(); }
  auto exitScope() -> void { _symtab.exitScope(); }

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