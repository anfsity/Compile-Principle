#pragma once

#include <string>

namespace ir {
class KoopaBuilder {
private:
  std::string buffer;

public:
  void append(std::string_view str) { buffer += str; }
  auto getResult() -> std::string const { return buffer; }
};
} // namespace ir