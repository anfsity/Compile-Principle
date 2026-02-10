/**
 * @file type.cppm
 * @brief Type system for the compiler's intermediate representation.
 */

module;

#include <fmt/core.h>
#include <map>
#include <memory>
#include <string>

export module ir.type;

export namespace type {

/**
 * @brief Base class for all types in the IR.
 */
class Type {
public:
  virtual ~Type() = default;

  /**
   * @brief Returns a string representation of the type.
   */
  virtual auto toKoopa() const -> std::string = 0;

  virtual auto is_int() const -> bool { return false; }
  virtual auto is_void() const -> bool { return false; }
  virtual auto is_ptr() const -> bool { return false; }
  virtual auto is_bool() const -> bool { return false; }
  virtual auto is_float() const -> bool { return false; }
  virtual auto is_array() const -> bool { return false; }
};

/**
 * @brief Represents a standard 32-bit integer type.
 */
class IntType : public Type {
public:
  auto toKoopa() const -> std::string override { return "i32"; }
  auto is_int() const -> bool override { return true; }

  /**
   * @brief Singleton getter for the IntType.
   */
  static auto get() -> std::shared_ptr<IntType> {
    static auto instance = std::make_shared<IntType>();
    return instance;
  }
};

/**
 * @brief Represents the void type (no return value).
 */
class VoidType : public Type {
public:
  auto toKoopa() const -> std::string override { return "void"; }
  auto is_void() const -> bool override { return true; }

  /**
   * @brief Singleton getter for the VoidType.
   */
  static auto get() -> std::shared_ptr<VoidType> {
    static auto instance = std::make_shared<VoidType>();
    return instance;
  }
};

/**
 * @brief Represents a boolean type.
 */
class BoolType : public Type {
public:
  auto toKoopa() const -> std::string override { return "bool"; }
  auto is_bool() const -> bool override { return true; }

  /**
   * @brief Singleton getter for the BoolType.
   */
  static auto get() -> std::shared_ptr<BoolType> {
    static auto instance = std::make_shared<BoolType>();
    return instance;
  }
};

/**
 * @brief Represents a floating-point type.
 */
class FloatType : public Type {
public:
  auto toKoopa() const -> std::string override { return "float"; }
  auto is_float() const -> bool override { return true; }

  /**
   * @brief Singleton getter for the FloatType.
   */
  static auto get() -> std::shared_ptr<FloatType> {
    static auto instance = std::make_shared<FloatType>();
    return instance;
  }
};

/**
 * @brief Represents a pointer type to another type.
 */
class PtrType : public Type {
public:
  std::shared_ptr<Type> target; ///< The type being pointed to.

  PtrType(std::shared_ptr<Type> t) : target(t) {};

  auto toKoopa() const -> std::string override {
    return fmt::format("*{}", target->toKoopa());
  }
  auto is_ptr() const -> bool override { return true; }

  static auto get(std::shared_ptr<Type> t) -> std::shared_ptr<PtrType> {
    return std::make_shared<PtrType>(t);
  }
};

class ArrayType : public Type {
public:
  std::shared_ptr<Type> base;
  int len;

  ArrayType(std::shared_ptr<Type> t, int _len) : base(t), len(_len) {}

  auto toKoopa() const -> std::string override {
    return fmt::format("[{}, {}]", base->toKoopa(), len);
  }
  auto is_array() const -> bool override { return true; }

  static auto get(std::shared_ptr<Type> _base, int _len)
      -> std::shared_ptr<ArrayType> {
    static std::map<std::pair<std::shared_ptr<Type>, int>,
                    std::shared_ptr<ArrayType>>
        cache;
    auto key = std::make_pair(_base, _len);
    if (cache.contains(key)) { return cache[key]; }
    auto instance = std::make_shared<ArrayType>(_base, _len);
    return cache[key] = instance;
  }
};

} // namespace type