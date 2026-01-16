/**
 * @file backend.hpp
 * @brief Definition of the RISC-V Target Code Generator.
 */
#pragma once

#include "ir/ir_builder.hpp"
#include "koopa.h"
#include <cassert>
#include <map>

namespace backend {

/**
 * @brief Generates RISC-V assembly from Koopa IR.
 */
class TargetCodeGen {
private:
  // clang-format off
  std::string buffer;       ///< Accumulates the generated assembly code.
  int stk_frame_size = 0;   ///< Total size of the current function's stack frame.
  int local_frame_size = 0; ///< Size of local variable storage area.
  int ra_size = 0;          ///< Space reserved for the Return Address (4 bytes) if needed.
  int args_size = 0;        ///< Space reserved for outgoing arguments on the stack.
  // clang-format on

  // the offset of value relative to sp
  std::map<const koopa_raw_value_t, int> stkMap;

public:
  /**
   * @brief Entry point for code generation from a Koopa program.
   */
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
  /**
   * @brief Generates code to load a value into a specific RISC-V register.
   */
  auto load_to(const koopa_raw_value_t &value, const std::string &reg) -> void;

  /**
   * @brief Resets the state of the generator, typically called before
   * processing a new function.
   */
  auto reset() -> void {
    stkMap.clear();
    stk_frame_size = ra_size = args_size = local_frame_size = 0;
  };

  // access raw function
  auto visit(koopa_raw_function_t func) -> void;

  // access raw basic block
  auto visit(koopa_raw_basic_block_t bb) -> void;

  // access raw insts
  auto visit(koopa_raw_value_t value) -> void;

  /** @name Specific Instruction Visitors
   *  @{
   */
  auto visit(const koopa_raw_return_t &ret) -> void;
  auto visit(const koopa_raw_binary_t &binary) -> void;
  auto visit(const koopa_raw_integer_t &integer) -> void;
  auto visit(const koopa_raw_jump_t &jump) -> void;
  auto visit(const koopa_raw_branch_t &branch) -> void;
  auto visit(const koopa_raw_load_t &load) -> void;
  auto visit(const koopa_raw_store_t &store) -> void;
  auto visit(const koopa_raw_call_t &call) -> void;
  auto visit(const koopa_raw_func_arg_ref_t &func_arg_ref) -> void;
  auto visit(const koopa_raw_global_alloc_t &global_alloc) -> void;
  /** @} */
};
} // namespace backend