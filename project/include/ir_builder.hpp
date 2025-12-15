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
  SymbolTable symtab;

public:
  auto append(std::string_view str) -> void { buffer += str; }
  auto newReg() -> std::string { return fmt::format("%{}", count_reg++); }
  auto newVar(std::string ident) -> std::string {
    return fmt::format("@{}_{}", ident, count_name++);
  };
  auto reset() -> void {
    count_reg = 0;
    count_name = 0;
  }
  auto getSymtable() -> SymbolTable & { return symtab; }
  auto enterScope() -> void { symtab.enterScope(); }
  auto exitScope() -> void { symtab.exitScope(); }

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