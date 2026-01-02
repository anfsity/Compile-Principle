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

namespace detail {
enum class SymbolKind { Var, Func };

struct Symbol {
  std::string name;
  std::string irName;
  std::shared_ptr<type::Type> type;
  SymbolKind kind;
  bool isConst;
  int constValue;
};
} // namespace detail

class SymbolTable {
private:
  //* scopes[0] indicates global scope
  std::vector<std::map<std::string, detail::Symbol>> scopes;

public:
  SymbolTable() { enterScope(); }

  auto enterScope() -> void { scopes.push_back({}); }

  auto exitScope() -> void {
    if (scopes.size() > 1u) {
      scopes.pop_back();
    }
  }

  auto isGlobalScope() const -> bool {
    return scopes.size() == 1u;
  }

  //FIXME: for debug
  auto size() const -> size_t {
    return scopes.size();
  }

  auto define(const std::string &name, const std::string &irName,
              std::shared_ptr<type::Type> type, detail::SymbolKind kind,
              bool isConst, int val = 0) -> void {

    if (scopes.back().contains(name)) {
      throw std::runtime_error("Semantic Error: Redefinition of " + name);
    }

    detail::Symbol sym{name, irName, type, kind, isConst, val};
    scopes.back()[name] = sym;
  }


  auto defineGlobal(const std::string &name, const std::string &irName,
              std::shared_ptr<type::Type> type, detail::SymbolKind kind,
              bool isConst, int val = 0) -> void {

    if (scopes[0].contains(name)) {
      throw std::runtime_error("Semantic Error: Redefinition of " + name);
    }

    detail::Symbol sym{name, irName, type, kind, isConst, val};
    scopes[0][name] = sym;
  }

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