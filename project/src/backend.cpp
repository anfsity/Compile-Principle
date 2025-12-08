#include "ir_builder.hpp"
#include "koopa.h"
#include <cassert>

class KoopaWrapper {
private:
  koopa_raw_program_t raw;
  koopa_program_t program;
  koopa_raw_program_builder_t builder;

public:
  KoopaWrapper(ir::KoopaBuilder &koopaBuilder) {
    koopa_error_code_t ret =
        koopa_parse_from_string(koopaBuilder.getResult().c_str(), &program);
    assert(ret == KOOPA_EC_SUCCESS && "parsing koopa ir failure");
    builder = koopa_new_raw_program_builder();
    raw = koopa_build_raw_program(builder, program);
  };

  ~KoopaWrapper() {
    koopa_delete_program(program);
    koopa_delete_raw_program_builder(builder);
  };

  // copy consturctor must be deleted, otherwise the copy behavior will cause raw break.
  KoopaWrapper(const KoopaWrapper &) = delete;
  KoopaWrapper &operator=(const KoopaWrapper &) = delete;

  // move constructor ?

  const koopa_raw_program_t &getRaw() const { return raw; }
};