/**
 * @file symbol_table.hpp
 * @brief Definition of the SymbolTable class for scope-based symbol management.
 *
 * This file provides the infrastructure for lexical scoping and symbol tracking
 * during the AST-to-IR generation phase. It supports nested scopes, constant
 * tracking, and IR name mapping.
 */
#pragma once

#include "Log/log.hpp"
#include "type.hpp"
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <vector>

namespace detail {
/**
 * @brief Categorizes the type of symbol.
 */
enum class SymbolKind { Var, Func };

/**
 * @brief Represents a single symbol in the source code.
 */
struct Symbol {
  std::string name;                 ///< Source name (e.g., "x")
  std::string irName;               ///< IR-level name (e.g., "@x_1")
  std::shared_ptr<type::Type> type; ///< Data type of the symbol
  SymbolKind kind;                  ///< Variable or Function
  bool isConst;                     ///< True if it's a compile-time constant
  int constValue; ///< The value of the constant, if applicable
};
} // namespace detail

/**
 * @brief A stack-based symbol table for handling lexical scopes.
 *
 * Manages symbol visibility across nested blocks and provides IR name
 * resolution. The 0-th index always represents the global scope.
 */
class SymbolTable {
private:
  //* scopes[0] indicates global scope
  /**
   * @brief A stack of maps, where each map represents a lexical scope.
   */
  std::vector<std::map<std::string, detail::Symbol>> scopes;

public:
  /**
   * @brief Construct a new Symbol Table and enter the global scope.
   */
  SymbolTable() { enterScope(); }

  /**
   * @brief Pushes a new scope onto the stack.
   */
  auto enterScope() -> void { scopes.push_back({}); }

  /**
   * @brief Pops the current scope from the stack.
   * @note Will not pop the global scope (level 0).
   */
  auto exitScope() -> void {
    if (scopes.size() > 1u) {
      scopes.pop_back();
    }
  }

  /**
   * @brief Checks if the current scope is the global scope.
   * @return true if we are in the global scope.
   */
  auto isGlobalScope() const -> bool { return scopes.size() == 1u; }

  /**
   * @brief Returns the current scope nesting depth (1 = global).
   */
  auto size() const -> size_t { return scopes.size(); }

  /**
   * @brief Defines a new symbol in the current (innermost) scope.
   *
   * @param name    Source code name.
   * @param irName  Generated IR name.
   * @param type    The type of the symbol.
   * @param kind    Var or Func.
   * @param isConst Whether it's constant.
   * @param val     Initial value if constant.
   */
  auto define(const std::string &name, const std::string &irName,
              std::shared_ptr<type::Type> type, detail::SymbolKind kind,
              bool isConst, int val = 0) -> void {

    if (scopes.back().contains(name)) {
      Log::panic("Semantic Error: Redefinition of " + name);
    }

    detail::Symbol sym{name, irName, type, kind, isConst, val};
    scopes.back()[name] = sym;
  }

  /**
   * @brief Defines a new symbol specifically in the global scope.
   */
  auto defineGlobal(const std::string &name, const std::string &irName,
                    std::shared_ptr<type::Type> type, detail::SymbolKind kind,
                    bool isConst, int val = 0) -> void {

    if (scopes[0].contains(name)) {
      Log::panic("Semantic Error: Redefinition of " + name);
    }

    detail::Symbol sym{name, irName, type, kind, isConst, val};
    scopes[0][name] = sym;
  }

  /**
   * @brief Searches for a symbol starting from the innermost scope outwards.
   *
   * @param name The source name to look up.
   * @return detail::Symbol* Pointer to the symbol if found, else nullptr.
   */
  auto lookup(const std::string &name) -> detail::Symbol * {
    for (auto &scope : std::views::reverse(scopes)) {
      auto it = scope.find(name);
      if (it != scope.end()) {
        return &(it->second);
      }
    }
    return nullptr;
  }
};