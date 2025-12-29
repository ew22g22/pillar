#ifndef PILLAR_CLASSFILE_FIELD_INFO_HPP
#define PILLAR_CLASSFILE_FIELD_INFO_HPP

#include "classfile_types.hpp"

namespace pillar {
enum struct field_info_access_flags : u2_t {
  PUBLIC = ACC_PUBLIC,
  PRIVATE = ACC_PRIVATE,
  PROTECTED = ACC_PROTECTED,
  STATIC = ACC_STATIC,
  FINAL = ACC_FINAL,
  VOLATILE = ACC_VOLATILE,
  TRANSIENT = ACC_TRANSIENT,
  SYNTHETIC = ACC_SYNTHETIC,
  ENUM = ACC_ENUM,
};

struct field_info {
  field_info_access_flags access_flags;
  u2_t name_index;
  u2_t descriptor_index;
  u2_t attributes_count;
};
} // namespace pillar

#endif
