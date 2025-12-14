// include/backend.hpp
#pragma once

#include "ir_builder.hpp"
#include "koopa.h"
#include <cassert>
#include <map>

namespace backend {
class KoopaWrapper {
private:
  koopa_raw_program_t raw;
  koopa_program_t program;
  koopa_raw_program_builder_t builder;

public:
  // I think koopawrapper should receive ir instead of koopabuilder ...
  KoopaWrapper(const std::string &IR) {
    koopa_error_code_t ret = koopa_parse_from_string(IR.c_str(), &program);
    assert(ret == KOOPA_EC_SUCCESS && "parsing koopa ir failure");
    builder = koopa_new_raw_program_builder();
    raw = koopa_build_raw_program(builder, program);
  };

  ~KoopaWrapper() noexcept {
    if (program)
      koopa_delete_program(program);
    if (builder)
      koopa_delete_raw_program_builder(builder);
  };

  // copy consturctor must be deleted, otherwise the copy behavior will cause
  // raw break.
  KoopaWrapper(const KoopaWrapper &) = delete;
  KoopaWrapper &operator=(const KoopaWrapper &) = delete;

  // move constructor ?
  KoopaWrapper(KoopaWrapper &&other) noexcept {
    raw = other.raw;
    program = other.program;
    builder = other.builder;
    // memory in other will be deconstruted, to avoid hang pointer, we need to
    // clear other's varibles.
    other.program = nullptr;
    other.builder = nullptr;
  }

  KoopaWrapper &operator=(KoopaWrapper &&other) {
    if (&other != this) {
      if (program)
        koopa_delete_program(program);
      if (builder)
        koopa_delete_raw_program_builder(builder);

      raw = other.raw;
      program = other.program;
      builder = other.builder;
      other.program = nullptr;
      other.builder = nullptr;
    }
    return *this;
  }

  const koopa_raw_program_t &getRaw() const { return raw; }
};

class TargetCodeGen {
private:
  std::string buffer;
  int stk_frame_size = 0;
  // the offset of value relative to sp
  std::map<const koopa_raw_value_t, int> stkMap;

public:
  auto visit(const koopa_raw_program_t &program) -> void;

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

  [[nodiscard]] auto getAssembly() -> std::string { return std::move(buffer); }

private:
  auto load_to(const koopa_raw_value_t &value, const std::string &reg) -> void;

  auto reset() -> void {
    stkMap.clear();
    stk_frame_size = 0;
  };

  // access raw function
  auto visit(koopa_raw_function_t func) -> void;

  // access raw basic block
  auto visit(koopa_raw_basic_block_t bb) -> void;

  // access raw insts
  auto visit(koopa_raw_value_t value) -> void;

  auto visit(koopa_raw_return_t ret) -> void;

  auto visit(const koopa_raw_binary_t &binary) -> void;

  auto visit(koopa_raw_integer_t integer) -> void;
};
} // namespace backend