// src/backend.cpp
#include "backend.hpp"
#include "ir_builder.hpp"
#include "koopa.h"
#include <cassert>
#include <fmt/core.h>
#include <span>

namespace backend {
template <typename ptrType> auto make_span(const koopa_raw_slice_t &slice) {
  return std::span<const ptrType>(
      reinterpret_cast<const ptrType *>(slice.buffer), slice.len);
}
} // namespace backend

using namespace backend;

void TargetCodeGen::visit(const koopa_raw_program_t &program) {
  // gobal values
  // FIXME: need process gobal values
  // function
  for (const auto func : make_span<koopa_raw_function_t>(program.funcs)) {
    visit(func);
  }
}

void TargetCodeGen::visit(koopa_raw_function_t func) {
  buffer += fmt::format("  .text\n");
  buffer += fmt::format("  .globl {}\n", func->name + 1);
  buffer += fmt::format("{}:\n", func->name + 1);
  for (const auto bb : make_span<koopa_raw_basic_block_t>(func->bbs)) {
    visit(bb);
  }
}

void TargetCodeGen::visit(koopa_raw_basic_block_t bb) {
  if (bb->name) {
    buffer += fmt::format("{}:\n", bb->name + 1);
  }
  for (const auto inst : make_span<koopa_raw_value_t>(bb->insts)) {
    visit(inst);
  }
}

void TargetCodeGen::visit(koopa_raw_value_t value) {
  const auto &kind = value->kind;
  switch (kind.tag) {
  case KOOPA_RVT_RETURN:
    visit(kind.data.ret);
    break;
  case KOOPA_RVT_INTEGER:
    visit(kind.data.integer);
    break;
  default:
    assert(false);
  }
}

void TargetCodeGen::visit(koopa_raw_return_t ret) {
  auto ret_val = ret.value;
  if (ret_val) {
    assert(ret_val->kind.tag == KOOPA_RVT_INTEGER);
    int32_t val = ret_val->kind.data.integer.value;
    buffer += fmt::format("  li a0, {}\n", val);
  }
  buffer += "  ret\n";
}

void TargetCodeGen::visit(koopa_raw_integer_t) {}