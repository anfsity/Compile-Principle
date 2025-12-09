// include/ir_builder.hpp
#pragma once

#include <string>

namespace ir {
class KoopaBuilder {
private:
  std::string buffer;

public:
  void append(std::string_view str) { buffer += str; }
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