// src/backend.cpp
#include "backend/backend.hpp"
#include "Log/log.hpp"
#include "ir/ir_builder.hpp"
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
  for (const auto value : make_span<koopa_raw_value_t>(program.values)) {
    visit(value);
  }

  for (const auto func : make_span<koopa_raw_function_t>(program.funcs)) {
    visit(func);
  }
}

auto TargetCodeGen::visit(koopa_raw_function_t func) -> void {
  if (func->bbs.len == 0)
    return;

  reset();

  bool has_callee = false;
  for (const auto bb : make_span<koopa_raw_basic_block_t>(func->bbs)) {
    for (const auto inst : make_span<koopa_raw_value_t>(bb->insts)) {
      // KOOPA_RTT_UNIT : Unit (void).
      if (inst->ty->tag != KOOPA_RTT_UNIT) {
        stkMap[inst] = local_frame_size;
        local_frame_size += 4;
      }
      if (inst->kind.tag == KOOPA_RVT_CALL) {
        has_callee = true;
        ra_size = 4;
        args_size = std::max<size_t>(args_size, inst->kind.data.call.args.len);
      }
    }
  }

  args_size = std::max<int>(args_size - 8, 0) * 4;
  stk_frame_size = local_frame_size + ra_size + args_size;

  Log::trace(fmt::format("stack frame size : {}", stk_frame_size));
  Log::trace(fmt::format("params frame size : {}", args_size));
  if (stk_frame_size % 16 != 0) {
    stk_frame_size = (stk_frame_size + 15) / 16 * 16;
  }

  buffer += fmt::format("\n  .text\n");
  buffer += fmt::format("  .globl {}\n", func->name + 1);
  buffer += fmt::format("{}:\n", func->name + 1);

  if (stk_frame_size > 0) {
    buffer += fmt::format("  addi sp, sp, -{}\n", stk_frame_size);
  }

  if (has_callee) {
    buffer += fmt::format("  sw ra, {}(sp)\n", stk_frame_size - 4);
  }

  for (auto &[key, val] : stkMap) {
    val += args_size;
  }

  for (size_t i = 0;
       const auto param : make_span<koopa_raw_value_t>(func->params)) {
    if (i < 8) {
      int offset = stkMap[param] = i * 4 + args_size;
      buffer += fmt::format("  sw a{}, {}(sp)\n", i, offset);
    } else {
      // FIXME:
      int offset = stk_frame_size + (i - 8) * 4;
      stkMap[param] = offset;
    }
    ++i;
  }

  // for (const auto param : make_span<koopa_raw_value_t>(func->params)) {
  //   Log::trace(param->name);
  //   visit(param);
  // }

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

auto TargetCodeGen::visit(koopa_raw_value_t value) -> void {
  const auto &kind = value->kind;
  switch (kind.tag) {
    // clang-format off
  case KOOPA_RVT_RETURN:  visit(kind.data.ret);         break;
  case KOOPA_RVT_INTEGER: visit(kind.data.integer); break;
  case KOOPA_RVT_STORE:   visit(kind.data.store);     break;
  case KOOPA_RVT_ALLOC:   /* nothing to do */                break;
  // clang-format on
  case KOOPA_RVT_BRANCH: {
    visit(kind.data.branch);
    break;
  }
  case KOOPA_RVT_JUMP: {
    visit(kind.data.jump);
    break;
  }
  case KOOPA_RVT_FUNC_ARG_REF: {
    visit(kind.data.func_arg_ref);
    break;
  }
  case KOOPA_RVT_CALL: {
    visit(kind.data.call);
    if (value->ty->tag == KOOPA_RTT_INT32) {
      int offset = stkMap[value];
      buffer += fmt::format("  sw a0, {}(sp)\n", offset);
    }
    break;
  }
  case KOOPA_RVT_GLOBAL_ALLOC: {
    buffer += "  .data\n";
    buffer += fmt::format("  .global {}\n", value->name + 1);
    buffer += fmt::format("{}:\n", value->name + 1);
    visit(kind.data.global_alloc);
    break;
  }
  case KOOPA_RVT_BINARY: {
    visit(kind.data.binary);
    // KOOPA_RTT_UNIT : Unit (void).
    if (value->ty->tag != KOOPA_RTT_UNIT) {
      int offset = stkMap[value];
      // store word : store the instruction's value to the stack (sp + offset)
      buffer += fmt::format("  sw t0, {}(sp)\n", offset);
    }
    break;
  }
  case KOOPA_RVT_LOAD: {
    visit(kind.data.load);
    // KOOPA_RTT_UNIT : Unit (void).
    if (value->ty->tag != KOOPA_RTT_UNIT) {
      int offset = stkMap[value];
      // store word : store the instruction's value to the stack (sp + offset)
      buffer += fmt::format("  sw t0, {}(sp)\n", offset);
    }
    break;
  }
  default: assert(false);
  }
}

auto TargetCodeGen::visit(const koopa_raw_branch_t &branch) -> void {
  load_to(branch.cond, "t0");
  buffer += fmt::format("  bnez t0, {}\n", branch.true_bb->name + 1);
  buffer += fmt::format("  j {}\n", branch.false_bb->name + 1);
}

auto TargetCodeGen::visit(const koopa_raw_jump_t &jump) -> void {
  std::string target_name = jump.target->name + 1;
  buffer += fmt::format("  j {}\n", target_name);
}

auto TargetCodeGen::visit(const koopa_raw_load_t &load) -> void {
  // load source must be alived here
  load_to(load.src, "t0");
  if (load.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
    buffer += "  lw t0, 0(t0)\n";
  }
}

auto TargetCodeGen::visit(const koopa_raw_store_t &store) -> void {
  // store src dest
  load_to(store.value, "t0");
  if (store.dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
    std::string name = store.dest->name + 1;
    buffer += fmt::format("  la t1, {}\n", name);
    buffer += "  sw t0, 0(t1)\n";
  } else if (stkMap.contains(store.dest)) {
    int offset = stkMap[store.dest];
    // need finish comment here
    // store word : store the instruction's value to the stack (sp + offset)
    buffer += fmt::format("  sw t0, {}(sp)\n", offset);
  }
}

auto TargetCodeGen::visit(const koopa_raw_return_t &ret) -> void {
  if (ret.value) {
    load_to(ret.value, "a0");
  }
  if (ra_size > 0) {
    buffer += fmt::format("  lw ra, {}(sp)\n", stk_frame_size - ra_size);
  }
  if (stk_frame_size > 0) {
    buffer += fmt::format("  addi sp, sp, {}\n", stk_frame_size);
  }
  buffer += "  ret\n";
}

auto TargetCodeGen::load_to(const koopa_raw_value_t &value,
                            const std::string &reg) -> void {
  switch (value->kind.tag) {
  case KOOPA_RVT_INTEGER: {
    int32_t val = value->kind.data.integer.value;
    // li t0, im
    buffer += fmt::format("  li {}, {}\n", reg, val);
    break;
  }
  case KOOPA_RVT_GLOBAL_ALLOC: {
    std::string var_name = value->name + 1;
    buffer += fmt::format("  la {}, {}\n", reg, var_name);
    break;
  }
  // fall through
  case KOOPA_RVT_CALL:
  case KOOPA_RVT_FUNC_ARG_REF:
  case KOOPA_RVT_BINARY:
  case KOOPA_RVT_LOAD:
  case KOOPA_RVT_ALLOC: {
    // {
    int offset = stkMap[value];
    // lw t0, offset(sp)
    buffer += fmt::format("  lw {}, {}(sp)\n", reg, offset);
    break;
  }
  default: {
    buffer += "Wait! Do you realy think you arguments should go here ?\n";
    break;
  }
  }
}

auto TargetCodeGen::visit(const koopa_raw_call_t &call) -> void {
  for (size_t i = 0;
       const auto param : make_span<koopa_raw_value_t>(call.args)) {
    if (i < 8) {
      std::string reg = fmt::format("a{}", i);
      load_to(param, reg);
    } else {
      // FIXME:
      load_to(param, "t0");
      int offset = (i - 8) * 4;
      buffer += fmt::format("  sw t0, {}(sp)\n", offset);
    }
    ++i;
  }
  buffer += fmt::format("  call {}\n", call.callee->name + 1);
}

auto TargetCodeGen::visit(const koopa_raw_func_arg_ref_t &) -> void {}

auto TargetCodeGen::visit(const koopa_raw_global_alloc_t &global_alloc)
    -> void {
  if (global_alloc.init->kind.tag == KOOPA_RVT_ZERO_INIT) {
    buffer += "  .zero 4\n";
  } else if (global_alloc.init->kind.tag == KOOPA_RVT_INTEGER) {
    buffer +=
        fmt::format("  .word {}\n", global_alloc.init->kind.data.integer.value);
  } else if (global_alloc.init->kind.tag == KOOPA_RVT_AGGREGATE) {
    /* array here */
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

auto TargetCodeGen::visit(const koopa_raw_integer_t &) -> void {}