// include/symbol_table.hpp
#pragma once

#include "type.hpp"
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <vector>

struct Symbol {
  std::string name;
  std::string irName;
  std::shared_ptr<type::Type> type;
  bool isConst;
  int constValue;
  int varValue;
};

class SymbolTable {
private:
  std::vector<std::map<std::string, Symbol>> scopes;

public:
  SymbolTable() { enterScope(); }

  auto enterScope() -> void { scopes.push_back({}); }

  auto exitScope() -> void {
    // clang-format off
      if (!scopes.empty()) scopes.pop_back();
    // clang-format on
  }

  auto define(const std::string &name, const std::string &irName,
              std::shared_ptr<type::Type> type, bool isConst, int val = 0)
      -> void {

    if (scopes.back().contains(name)) {
      throw std::runtime_error("Semantic Error: Redefinition of " + name);
    }

    Symbol sym{name, irName, type, isConst, val, 0};
    scopes.back()[name] = sym;
  }

  auto lookup(const std::string &name) -> Symbol* {
    for (auto &scope : std::views::reverse(scopes)) {
      auto it = scope.find(name);
      if (it != scope.end()) {
        return &(it->second);
      }
    }
    return nullptr;
  }
};