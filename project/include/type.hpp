// include/type.hpp
#pragma once

#include <fmt/core.h>
#include <memory>
#include <string>
#include <vector>

namespace type {
class Type {
public:
  virtual ~Type() = default;
  virtual auto debug() const -> std::string = 0;
  virtual auto is_int() const -> bool { return false; }
  virtual auto is_void() const -> bool { return false; }
  virtual auto is_ptr() const -> bool { return false; }
  virtual auto is_bool() const -> bool { return false; }
};

class IntType : public Type {
  auto debug() const -> std::string override { return "int"; }
  auto is_int() const -> bool override { return true; }
  static auto get() -> std::shared_ptr<IntType> {
    static auto instance = std::make_shared<IntType>();
    return instance;
  }
};

class VoidType : public Type {
  auto debug() const -> std::string override { return "void"; }
  auto is_void() const -> bool override { return true; }
  static auto get() -> std::shared_ptr<VoidType> {
    static auto instance = std::make_shared<VoidType>();
    return instance;
  }
};

class BoolType : public Type {
  auto debug() const -> std::string override { return "bool"; }
  auto is_bool() const -> bool override { return true; }
  static auto get() -> std::shared_ptr<BoolType> {
    static auto instance = std::make_shared<BoolType>();
    return instance;
  }
};

class PtrType : public Type {
  std::shared_ptr<Type> target;
  PtrType(std::shared_ptr<Type> t) : target(t) {};
  auto debug() const -> std::string override { return fmt::format("*{}", target->debug()); }
  auto is_ptr() const -> bool override { return true; }
};

} // namespace type