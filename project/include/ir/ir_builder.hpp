// include/ir_builder.hpp
#pragma once

#include "symbol_table.hpp"
#include <fmt/core.h>
#include <string>

namespace ir {
class KoopaBuilder {
private:
  std::string buffer;
  int count_reg = 0;
  int count_name = 0;
  int count_label = 0;
  bool _is_block_closed = false;
  SymbolTable _symtab;

public:
  auto append(std::string_view str) -> void { buffer += str; }
  auto newReg() -> std::string { return fmt::format("%{}", count_reg++); }
  auto newVar(std::string_view ident) -> std::string {
    return fmt::format("@{}_{}", ident, count_name++);
  };

  auto allocLabelId() -> int { return count_label++; }
  auto newLabel(std::string_view prefix, int id) -> std::string {
    return fmt::format("%{}_{}", prefix, id);
  }
  auto allocUniqueLabel(std::string_view prefix) -> std::string {
    return newLabel(prefix, allocLabelId());
  }

  auto isBlockClose() -> bool { return _is_block_closed; }
  auto setBlockClose() -> void { _is_block_closed = true; }
  auto clearBlockClose() -> void { _is_block_closed = false; }

  auto reset() -> void {
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