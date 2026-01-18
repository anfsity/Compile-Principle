/**
 * @file koopawrapper.cppm
 * @brief RAII wrapper for Koopa IR parsing and raw program building.
 *
 * This file provides a C++ wrapper around the Koopa C API to ensure 
 * proper resource management (RAII) for Koopa programs and builders.
 */

module;

#include "koopa.h"
#include <cassert>
#include <string>

export module koopawrapper;

import ir_builder;

export namespace backend {

/**
 * @brief Manages the lifecycle of Koopa IR objects.
 *
 * It parses an IR string, converts it to a "raw" format suitable for 
 * backend processing, and handles the cleanup of underlying C structures.
 */
class KoopaWrapper {
private:
  koopa_raw_program_t raw;              ///< The "raw" Koopa program used for backend traversal.
  koopa_program_t program;              ///< The parsed Koopa program object.
  koopa_raw_program_builder_t builder;  ///< The builder used to generate the raw program.

public:
  /**
   * @brief Constructs a KoopaWrapper by parsing an IR string.
   * 
   * @param IR The Koopa IR text to parse.
   * @throw Assertion failure if parsing fails.
   */
  KoopaWrapper(const std::string &IR) {
    koopa_error_code_t ret = koopa_parse_from_string(IR.c_str(), &program);
    assert(ret == KOOPA_EC_SUCCESS && "parsing koopa ir failure");
    builder = koopa_new_raw_program_builder();
    raw = koopa_build_raw_program(builder, program);
  };

  /**
   * @brief Cleans up Koopa C API resources.
   */
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

  /**
   * @brief Move constructor transfers ownership of Koopa resources.
   */
  KoopaWrapper(KoopaWrapper &&other) noexcept {
    raw = other.raw;
    program = other.program;
    builder = other.builder;
    // memory in other will be deconstruted, to avoid hang pointer, we need to
    // clear other's varibles.
    other.program = nullptr;
    other.builder = nullptr;
  }

  /**
   * @brief Move assignment operator.
   */
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

  /**
   * @brief Gets the raw program structure.
   * @return const koopa_raw_program_t& Reference to the raw program.
   */
  const koopa_raw_program_t &getRaw() const { return raw; }
};
}