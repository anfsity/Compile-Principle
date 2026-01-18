/**
 * @file backend.cpp
 * @brief Implementation of the RISC-V backend for the Koopa IR.
 *
 * This file contains the TargetCodeGen class which traverses the Koopa IR
 * and generates corresponding RISC-V assembly code.
 *
 * ### Koopa IR Architecture Overview
 * Koopa's IR is structured as a tree-like hierarchy of "raw" components:
 * - @ref koopa_raw_program_t: The root of the IR, containing global values and
 * functions.
 * - @ref koopa_raw_function_t: Represents a single function, containing
 * parameters and basic blocks.
 * - @ref koopa_raw_basic_block_t: A sequence of instructions ending with a
 * terminator (br, jump, ret).
 * - @ref koopa_raw_value_t: The fundamental unit in Koopa. It can represent:
 *   - Instructions (Binary, Load, Store, Call, etc.)
 *   - Constants (Integer)
 *   - Function Parameters
 *   - Global Allocations
 *
 * Values in Koopa are "typed" and have a "kind". The kind determines what data
 * the value holds.
 *
 * ### Koopa Naming Conventions (Tags & Enums)
 * Koopa's C API uses specific prefixes to categorize its enumeration tags:
 * - **`RVT` (Raw Value Tag)**: Defines the *behavior* or *identity* of a
 * `koopa_raw_value_t`.
 *   - Examples: `KOOPA_RVT_INTEGER` (constant), `KOOPA_RVT_BINARY`
 * (instruction), `KOOPA_RVT_ALLOC` (stack allocation).
 *   - Usage: `value->kind.tag` is checked against these tags.
 * - **`RTT` (Raw Type Tag)**: Defines the *data type* of a value or instruction
 * result.
 *   - Examples: `KOOPA_RTT_INT32` (32-bit integer), `KOOPA_RTT_UNIT` (void/no
 * return), `KOOPA_RTT_POINTER` (memory address).
 *   - Usage: `value->ty->tag` describes the type of the value produced.
 * - **`RBO` (Raw Binary Operator)**: Specifies the operation in a binary
 * expression.
 *   - Examples: `KOOPA_RBO_ADD` (+), `KOOPA_RBO_SUB` (-), `KOOPA_RBO_EQ` (==),
 * `KOOPA_RBO_OR` (logical OR).
 *   - Usage: Found within `koopa_raw_binary_t::op`.
 * - **`RSI` (Raw Slice)**: Not usually a tag, but refers to
 * `koopa_raw_slice_t`, a generic array/list structure.
 *
 * ### Detailed Tag Reference
 * #### Raw Value Tags (RVT) - `value->kind.tag`
 * | Tag | Description | Data Access |
 * | :--- | :--- | :--- |
 * | `KOOPA_RVT_INTEGER` | Constant 32-bit integer | `kind.data.integer.value` |
 * | `KOOPA_RVT_ZERO_INIT` | Zero-initialized global/aggregate | (None) |
 * | `KOOPA_RVT_AGGREGATE` | Array/Aggregate initializer |
 * `kind.data.aggregate.elems` | | `KOOPA_RVT_GLOBAL_ALLOC`| Global variable
 * definition | `kind.data.global_alloc.init` | | `KOOPA_RVT_FUNC_ARG_REF`|
 * Reference to function parameter | `kind.data.func_arg_ref.index` | |
 * `KOOPA_RVT_ALLOC` | Local stack allocation (`@x = alloc i32`) | (None,
 * returns pointer) | | `KOOPA_RVT_LOAD` | Memory read (`%0 = load @x`) |
 * `kind.data.load.src` | | `KOOPA_RVT_STORE` | Memory write (`store %0, @x`) |
 * `kind.data.store.value`, `.dest` | | `KOOPA_RVT_GET_ELEM_PTR`| Pointer
 * arithmetic for arrays | `kind.data.get_elem_ptr.src`, `.index` | |
 * `KOOPA_RVT_GET_PTR` | Pointer arithmetic for pointers |
 * `kind.data.get_ptr.src`, `.index` | | `KOOPA_RVT_BINARY` | Binary
 * arithmetic/logical op | `kind.data.binary.lhs`, `.rhs`, `.op` | |
 * `KOOPA_RVT_BRANCH` | Conditional jump | `kind.data.branch.cond`, `.true_bb`,
 * `.false_bb` | | `KOOPA_RVT_JUMP` | Unconditional jump |
 * `kind.data.jump.target` | | `KOOPA_RVT_CALL` | Function call |
 * `kind.data.call.callee`, `.args` | | `KOOPA_RVT_RETURN` | Return instruction
 * | `kind.data.ret.value` |
 *
 * #### Raw Type Tags (RTT) - `value->ty->tag`
 * | Tag | Description |
 * | :--- | :--- |
 * | `KOOPA_RTT_INT32` | 32-bit signed integer (`i32`) |
 * | `KOOPA_RTT_UNIT` | Void type (used for instructions without resultslike
 * `store`, `br`) | | `KOOPA_RTT_ARRAY` | Array type (e.g., `[i32, 10]`) | |
 * `KOOPA_RTT_POINTER` | Pointer type (e.g., `*i32`) | | `KOOPA_RTT_FUNCTION` |
 * Function signature type |
 *
 * ### Stack Frame Layout
 * The TargetCodeGen uses a simple stack allocation strategy:
 *
 * ```
 * |        ...        |
 * +-------------------+  <-- High Address (Previous Frame)
 * |  Caller's Args    |  (Arguments 9 and above)
 * +-------------------+  <-- Caller's SP
 * |  Saved RA         |  (if function calls others)
 * +-------------------+
 * |  Local Variables  |  (Space for instructions with results)
 * +-------------------+
 * |  Outgoing Args    |  (Space for arguments to callees, if > 8)
 * +-------------------+  <-- Low Address (Current SP)
 * ```
 *
 * - `args_size`: Maximum space required for arguments passed to any function
 * called by this function.
 * - `local_frame_size`: Total size of all local variables and intermediate
 * results.
 * - `stk_frame_size`: Total size of the current stack frame, aligned to 16
 * bytes (RISC-V calling convention).
 */

module;

#include "koopa.h"
#include <cassert>
#include <fmt/core.h>
#include <span>
#include <string>

module backend;

import ir_builder;
import log;

namespace backend {
/**
 * @brief Converts a raw Koopa slice (void**) into a typed C++ span.
 *
 * @tparam ptrType The Koopa type, which SHOULD be a pointer typedef.
 *         (e.g., koopa_raw_function_t, NOT koopa_raw_function_data_t)
 *
 * @note Memory Layout Explanation:
 * Koopa's slice.buffer is 'const void**', meaning it is an array of pointers.
 * We reinterpret_cast it to 'const ptrType*', effectively treating it as:
 *
 *    [ void* ] [ void* ] ...  (Raw View)
 *       |         |
 *       v         v
 *    [ Func* ] [ Func* ] ...  (Typed View via Span)
 *
 * This allows us to iterate using: for (koopa_raw_function_t func : span) ...
 */
template <typename ptrType> auto make_span(const koopa_raw_slice_t &slice) {
  return std::span<const ptrType>(
      reinterpret_cast<const ptrType *>(slice.buffer), slice.len);
}
} // namespace backend

using namespace backend;

auto TargetCodeGen::visit(const koopa_raw_program_t &program) -> void {
  for (const auto value : make_span<koopa_raw_value_t>(program.values)) {
    visit(value);
  }

  for (const auto func : make_span<koopa_raw_function_t>(program.funcs)) {
    visit(func);
  }
}

/**
 * @brief Generates assembly for a Koopa function.
 *
 * This method first performs a pre-pass to calculate the required stack frame
 * size, then generates the prologue, visits each basic block, and (implicitly)
 * the epilogue.
 *
 * @param func The Koopa function to process.
 */
auto TargetCodeGen::visit(koopa_raw_function_t func) -> void {
  if (func->bbs.len == 0)
    return;

  reset();

  // --- Stack Frame Calculation (Pre-pass) ---
  bool has_callee = false;
  for (const auto bb : make_span<koopa_raw_basic_block_t>(func->bbs)) {
    for (const auto inst : make_span<koopa_raw_value_t>(bb->insts)) {
      // If the instruction produces a value (not void), allocate space on
      // stack. In this simple backend, every value-producing instruction gets
      // its own slot.
      if (inst->ty->tag != KOOPA_RTT_UNIT) {
        stkMap[inst] = local_frame_size;
        local_frame_size += 4;
      }
      // If this function calls another, we need to save RA and potentially
      // allocate space for outgoing arguments.
      if (inst->kind.tag == KOOPA_RVT_CALL) {
        has_callee = true;
        ra_size = 4;
        // Keep track of the maximum number of arguments in any call.
        args_size = std::max<size_t>(args_size, inst->kind.data.call.args.len);
      }
    }
  }

  // RISC-V convention: first 8 args are in a0-a7, rest on stack.
  // args_size here becomes the number of 4-byte slots needed for outgoing args.
  args_size = std::max<int>(args_size - 8, 0) * 4;
  stk_frame_size = local_frame_size + ra_size + args_size;

  // Align stack frame to 16 bytes as per RISC-V ABI.
  if (stk_frame_size % 16 != 0) {
    stk_frame_size = (stk_frame_size + 15) / 16 * 16;
  }

  // --- Function Prologue ---
  buffer += fmt::format("\n  .text\n");
  buffer += fmt::format("  .globl {}\n", func->name + 1);
  buffer += fmt::format("{}:\n", func->name + 1);

  if (stk_frame_size > 0) {
    buffer += fmt::format("  addi sp, sp, -{}\n", stk_frame_size);
  }

  if (has_callee) {
    // Save RA at the top of the frame (below the caller's frame).
    buffer += fmt::format("  sw ra, {}(sp)\n", stk_frame_size - 4);
  }

  // Offset local variable storage by the size allocated for outgoing arguments.
  for (auto &[key, val] : stkMap) {
    val += args_size;
  }

  // --- Parameter Handling ---
  for (size_t i = 0;
       const auto param : make_span<koopa_raw_value_t>(func->params)) {
    if (i < 8) {
      // Store input parameters (a0-a7) into their allocated stack slots.
      int offset = stkMap[param] = i * 4 + args_size;
      buffer += fmt::format("  sw a{}, {}(sp)\n", i, offset);
    } else {
      // Parameters passed on stack by caller are located ABOVE the current SP.
      int offset = stk_frame_size + (i - 8) * 4;
      stkMap[param] = offset;
    }
    ++i;
  }

  // --- Function Body ---
  for (const auto bb : make_span<koopa_raw_basic_block_t>(func->bbs)) {
    visit(bb);
  }
}

auto TargetCodeGen::visit(koopa_raw_basic_block_t bb) -> void {
  if (bb->name) {
    buffer += fmt::format("{}:\n", bb->name + 1);
  }
  for (const auto inst : make_span<koopa_raw_value_t>(bb->insts)) {
    visit(inst);
  }
}

/**
 * @brief Main dispatcher for Koopa values.
 *
 * In Koopa, almost everything (instructions, constants, refs) is a "value".
 * This method identifies the kind of value and delegates to specific visit
 * methods.
 */
auto TargetCodeGen::visit(koopa_raw_value_t value) -> void {
  const auto &kind = value->kind;
  switch (kind.tag) {

  case KOOPA_RVT_RETURN: {
    visit(kind.data.ret);
    break;
  }

  case KOOPA_RVT_INTEGER: {
    visit(kind.data.integer);
    break;
  }

  case KOOPA_RVT_STORE: {
    visit(kind.data.store);
    break;
  }

  case KOOPA_RVT_ALLOC: {
    /* Stack space already reserved in function pre-pass */
    break;
  }

  case KOOPA_RVT_BRANCH: {
    visit(kind.data.branch);
    break;
  }

  case KOOPA_RVT_JUMP: {
    visit(kind.data.jump);
    break;
  }

  case KOOPA_RVT_FUNC_ARG_REF: {
    // Reference to a function argument.
    visit(kind.data.func_arg_ref);
    break;
  }

  case KOOPA_RVT_CALL: {
    visit(kind.data.call);
    // If the function returns an int, it's in a0. Save it to the stack slot
    // assigned to this 'call' value.
    if (value->ty->tag == KOOPA_RTT_INT32) {
      int offset = stkMap[value];
      buffer += fmt::format("  sw a0, {}(sp)\n", offset);
    }
    break;
  }

  case KOOPA_RVT_GLOBAL_ALLOC: {
    // Global variable allocation in the .data section.
    buffer += "  .data\n";
    buffer += fmt::format("  .global {}\n", value->name + 1);
    buffer += fmt::format("{}:\n", value->name + 1);
    visit(kind.data.global_alloc);
    break;
  }

  case KOOPA_RVT_BINARY: {
    visit(kind.data.binary);
    // Binary operations result in a value in t0. Save it to stack.
    if (value->ty->tag != KOOPA_RTT_UNIT) {
      int offset = stkMap[value];
      buffer += fmt::format("  sw t0, {}(sp)\n", offset);
    }
    break;
  }

  case KOOPA_RVT_LOAD: {
    visit(kind.data.load);
    // Load result is in t0. Save it to stack.
    if (value->ty->tag != KOOPA_RTT_UNIT) {
      int offset = stkMap[value];
      buffer += fmt::format("  sw t0, {}(sp)\n", offset);
    }
    break;
  }

  default: assert(false);
  }
}

auto TargetCodeGen::visit(const koopa_raw_branch_t &branch) -> void {
  load_to(branch.cond, "t0");
  // bnez: branch if not equal to zero.
  buffer += fmt::format("  bnez t0, {}\n", branch.true_bb->name + 1);
  buffer += fmt::format("  j {}\n", branch.false_bb->name + 1);
}

auto TargetCodeGen::visit(const koopa_raw_jump_t &jump) -> void {
  std::string target_name = jump.target->name + 1;
  buffer += fmt::format("  j {}\n", target_name);
}

auto TargetCodeGen::visit(const koopa_raw_load_t &load) -> void {
  // src is a pointer value (either a local alloc or a global).
  load_to(load.src, "t0");
  // If it's a global, t0 now contains the address. We need to load from it.
  if (load.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
    buffer += "  lw t0, 0(t0)\n";
  }
  // Note: if it was a local alloc, load_to already performed the lw t0,
  // offset(sp) because local allocs are just pointers to stack slots.
}

auto TargetCodeGen::visit(const koopa_raw_store_t &store) -> void {
  // Store 'value' into 'dest'.
  load_to(store.value, "t0");
  if (store.dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
    std::string name = store.dest->name + 1;
    buffer += fmt::format("  la t1, {}\n", name);
    buffer += "  sw t0, 0(t1)\n";
  } else if (stkMap.contains(store.dest)) {
    int offset = stkMap[store.dest];
    // Store the value into the destination's stack slot.
    buffer += fmt::format("  sw t0, {}(sp)\n", offset);
  }
}

/**
 * @brief Generates epilogue and return instruction.
 */
auto TargetCodeGen::visit(const koopa_raw_return_t &ret) -> void {
  if (ret.value) {
    // Return values are placed in a0.
    load_to(ret.value, "a0");
  }
  // Function Epilogue: Restore RA and SP.
  if (ra_size > 0) {
    buffer += fmt::format("  lw ra, {}(sp)\n", stk_frame_size - ra_size);
  }
  if (stk_frame_size > 0) {
    buffer += fmt::format("  addi sp, sp, {}\n", stk_frame_size);
  }
  buffer += "  ret\n";
}

/**
 * @brief Loads a Koopa value into a RISC-V register.
 *
 * Depending on the kind of value, this may involve loading a constant,
 * loading from a stack slot, or calculating a global address.
 *
 * @param value The Koopa value to load.
 * @param reg   The target register name (e.g., "t0").
 */
auto TargetCodeGen::load_to(const koopa_raw_value_t &value,
                            const std::string &reg) -> void {
  switch (value->kind.tag) {

  case KOOPA_RVT_INTEGER: {
    // Constant integer.
    int32_t val = value->kind.data.integer.value;
    buffer += fmt::format("  li {}, {}\n", reg, val);
    break;
  }

  case KOOPA_RVT_GLOBAL_ALLOC: {
    // Global variable address.
    std::string var_name = value->name + 1;
    buffer += fmt::format("  la {}, {}\n", reg, var_name);
    break;
  }

  // Local allocations and instruction results are all stored in the stack
  // frame.
  case KOOPA_RVT_CALL:
  case KOOPA_RVT_FUNC_ARG_REF:
  case KOOPA_RVT_BINARY:
  case KOOPA_RVT_LOAD:
  case KOOPA_RVT_ALLOC: {
    int offset = stkMap[value];
    buffer += fmt::format("  lw {}, {}(sp)\n", reg, offset);
    break;
  }

  default: {
    buffer += "Wait! Do you realy think you arguments should go here ?\n";
    break;
  }
  }
}

/**
 * @brief Generates assembly for a function call.
 *
 * Follows RISC-V calling convention: first 8 arguments in a0-a7,
 * the rest on the stack (at the very bottom of the current frame).
 *
 * @param call The Koopa call instruction data.
 */
auto TargetCodeGen::visit(const koopa_raw_call_t &call) -> void {
  for (size_t i = 0;
       const auto param : make_span<koopa_raw_value_t>(call.args)) {
    if (i < 8) {
      // First 8 args go into registers a0-a7.
      std::string reg = fmt::format("a{}", i);
      load_to(param, reg);
    } else {
      // Args 9+ go onto the stack at the very bottom of the current frame.
      load_to(param, "t0");
      int offset = (i - 8) * 4;
      buffer += fmt::format("  sw t0, {}(sp)\n", offset);
    }
    ++i;
  }
  // The callee's name starts with '@', so skip first char.
  buffer += fmt::format("  call {}\n", call.callee->name + 1);
}

auto TargetCodeGen::visit(const koopa_raw_func_arg_ref_t &) -> void {}

/**
 * @brief Handles global variable allocation.
 */
auto TargetCodeGen::visit(const koopa_raw_global_alloc_t &global_alloc)
    -> void {
  if (global_alloc.init->kind.tag == KOOPA_RVT_ZERO_INIT) {
    // Uninitialized/Zero-initialized global.
    buffer += "  .zero 4\n";
  } else if (global_alloc.init->kind.tag == KOOPA_RVT_INTEGER) {
    // Constant-initialized global.
    buffer +=
        fmt::format("  .word {}\n", global_alloc.init->kind.data.integer.value);
  } else if (global_alloc.init->kind.tag == KOOPA_RVT_AGGREGATE) {
    /* TODO: Implement aggregate (array) initialization */
  }
}

/**
 * @brief Generates assembly for binary operations.
 *
 * This method uses the Raw Binary Operator (RBO) tag to determine
 * which RISC-V assembly instruction to emit.
 *
 * @param binary The Koopa binary instruction data.
 */
auto TargetCodeGen::visit(const koopa_raw_binary_t &binary) -> void {
  load_to(binary.lhs, "t0");
  load_to(binary.rhs, "t1");

  // clang-format off
  // Dispatch based on the KOOPA_RBO enum tag.
  switch (binary.op) {
  case KOOPA_RBO_ADD: buffer += fmt::format("  add t0, t0, t1\n"); break;
  case KOOPA_RBO_SUB: buffer += fmt::format("  sub t0, t0, t1\n"); break;
  case KOOPA_RBO_MUL: buffer += fmt::format("  mul t0, t0, t1\n"); break;
  case KOOPA_RBO_DIV: buffer += fmt::format("  div t0, t0, t1\n"); break;
  case KOOPA_RBO_MOD: buffer += fmt::format("  rem t0, t0, t1\n"); break;
  case KOOPA_RBO_AND: buffer += fmt::format("  and t0, t0, t1\n"); break;
  case KOOPA_RBO_OR:  buffer += fmt::format("  or  t0, t0, t1\n"); break;
  case KOOPA_RBO_XOR: buffer += fmt::format("  xor t0, t0, t1\n"); break;
  // shift operation
  case KOOPA_RBO_SHL: buffer += fmt::format("  sll t0, t0, t1\n"); break;
  case KOOPA_RBO_SHR: buffer += fmt::format("  srl t0, t0, t1\n"); break;
  case KOOPA_RBO_SAR: buffer += fmt::format("  sra t0, t0, t1\n"); break;
  // complex instruction
  case KOOPA_RBO_LT:  buffer += fmt::format("  slt t0, t0, t1\n"); break;
  case KOOPA_RBO_GT:  buffer += fmt::format("  sgt t0, t0, t1\n"); break;
  case KOOPA_RBO_LE:
    buffer += fmt::format("  sgt t0, t0, t1\n");
    buffer += fmt::format("  seqz t0, t0\n");
    break;
  case KOOPA_RBO_GE:
    buffer += fmt::format("  slt t0, t0, t1\n");
    buffer += fmt::format("  seqz t0, t0\n");
    break;
  case KOOPA_RBO_EQ:
    buffer += fmt::format("  xor t0, t0, t1\n");
    buffer += fmt::format("  seqz t0, t0\n");
    break;
  case KOOPA_RBO_NOT_EQ:
    buffer += fmt::format("  xor t0, t0, t1\n");
    buffer += fmt::format("  snez t0, t0\n");
    break;
  default: assert(false);
  }
  // clang-format on
}

auto TargetCodeGen::visit(const koopa_raw_integer_t &) -> void {}