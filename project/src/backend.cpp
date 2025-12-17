// src/backend.cpp
#include "backend.hpp"
#include "ir_builder.hpp"
#include "koopa.h"
#include <cassert>
#include <fmt/core.h>
#include <span>

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
  // gobal values
  // FIXME: need process gobal values
  for (const auto func : make_span<koopa_raw_function_t>(program.funcs)) {
    visit(func);
  }
}

auto TargetCodeGen::visit(koopa_raw_function_t func) -> void {
  // FIXME: have not finished multiple function code gen yet
  if (func->bbs.len == 0)
    return;

  reset();
  for (const auto bb : make_span<koopa_raw_basic_block_t>(func->bbs)) {
    for (const auto inst : make_span<koopa_raw_value_t>(bb->insts)) {
      // KOOPA_RTT_UNIT : Unit (void).
      if (inst->ty->tag != KOOPA_RTT_UNIT) {
        stkMap[inst] = stk_frame_size;
        stk_frame_size += 4;
      }
    }
  }

  if (stk_frame_size % 16 != 0) {
    stk_frame_size = (stk_frame_size + 15) / 16 * 16;
  }

  buffer += fmt::format("  .text\n");
  buffer += fmt::format("  .globl {}\n", func->name + 1);
  buffer += fmt::format("{}:\n", func->name + 1);

  if (stk_frame_size > 0) {
    buffer += fmt::format("  addi sp, sp, -{}\n", stk_frame_size);
  }

  for (const auto bb : make_span<koopa_raw_basic_block_t>(func->bbs)) {
    visit(bb);
  }
}

auto TargetCodeGen::visit(koopa_raw_basic_block_t bb) -> void {
  if (bb->name) {
    buffer += fmt::format(".{}:\n", bb->name + 1);
  }
  for (const auto inst : make_span<koopa_raw_value_t>(bb->insts)) {
    visit(inst);
  }
}

auto TargetCodeGen::visit(koopa_raw_value_t value) -> void {
  const auto &kind = value->kind;
  switch (kind.tag) {
    // clang-format off
  case KOOPA_RVT_RETURN:  visit(kind.data.ret);         break;
  case KOOPA_RVT_INTEGER: visit(kind.data.integer); break;
  case KOOPA_RVT_STORE:   visit(kind.data.store);     break;
  case KOOPA_RVT_ALLOC:   /* nothing to do */                break;
  // clang-format on
  case KOOPA_RVT_BINARY:
    visit(kind.data.binary);
    // KOOPA_RTT_UNIT : Unit (void).
    if (value->ty->tag != KOOPA_RTT_UNIT) {
      int offset = stkMap[value];
      // store word : store the instruction's value to the stack (sp + offset)
      buffer += fmt::format("  sw t0, {}(sp)\n", offset);
    }
    break;
  case KOOPA_RVT_LOAD:
    visit(kind.data.load);
    // KOOPA_RTT_UNIT : Unit (void).
    if (value->ty->tag != KOOPA_RTT_UNIT) {
      int offset = stkMap[value];
      // store word : store the instruction's value to the stack (sp + offset)
      buffer += fmt::format("  sw t0, {}(sp)\n", offset);
    }
    break;
  default: assert(false);
  }
}

auto TargetCodeGen::visit(const koopa_raw_load_t &load) -> void {
  // load source must be alived here
  load_to(load.src, "t0");
}

auto TargetCodeGen::visit(const koopa_raw_store_t &store) -> void {
  // store src dest
  load_to(store.value, "t0");
  int offset = stkMap[store.dest];
  // store word : store the instruction's value to the stack (sp + offset)
  buffer += fmt::format("  sw t0, {}(sp)\n", offset);
}

auto TargetCodeGen::visit(koopa_raw_return_t ret) -> void {
  if (ret.value) {
    load_to(ret.value, "a0");
  }
  if (stk_frame_size > 0) {
    buffer += fmt::format("  addi sp, sp, {}\n", stk_frame_size);
  }
  buffer += "  ret\n";
}

auto TargetCodeGen::load_to(const koopa_raw_value_t &value,
                            const std::string &reg) -> void {
  if (value->kind.tag == KOOPA_RVT_INTEGER) {
    int32_t val = value->kind.data.integer.value;
    // li t0, im
    buffer += fmt::format("  li {}, {}\n", reg, val);
  } else {
    int offset = stkMap[value];
    // lw t0, offset(sp)
    buffer += fmt::format("  lw {}, {}(sp)\n", reg, offset);
  }
}

auto TargetCodeGen::visit(const koopa_raw_binary_t &binary) -> void {
  load_to(binary.lhs, "t0");
  load_to(binary.rhs, "t1");

  // clang-format off
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

auto TargetCodeGen::visit(koopa_raw_integer_t) -> void {}