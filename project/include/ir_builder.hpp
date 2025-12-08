#pragma once

#include <string>

namespace ir {
class KoopaBuilder {
private:
  std::string buffer;

public:
  void append(std::string_view str) { buffer += str; }
  std::string getResult() const { return buffer; }
};
} // namespace ir