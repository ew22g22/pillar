#ifndef PILLAR_CLASSFILE_CLASSFILE_HPP
#define PILLAR_CLASSFILE_CLASSFILE_HPP

#include <cstddef>
#include <expected>
#include <span>

#include "classfile_types.hpp"
#include "cp_info.hpp"

namespace pillar {
enum struct classfile_reader_error_reason {
  NOT_ENOUGH_BYTES,
  INVALID_MAGIC_NUMBER,
  INVALID_CP_TAG,
};

struct classfile_reader_error {
  inline static auto constexpr NO_BYTE_INDEX
      = std::numeric_limits<std::span<std::byte>::size_type>::max();

  classfile_reader_error_reason reason;
  std::span<std::byte>::size_type byte_index;
};

struct classfile {
  u4_t magic = 0xCAFEBABE;
  u2_t minor_version;
  u2_t major_version;
  u2_t constant_pool_count;
  std::vector<cp_info_t> constant_pool;
  u2_t access_flags; /* TODO: Make this an enum struct */
  u2_t this_class;
  u2_t super_class;
  u2_t interfaces_count;
  std::vector<u2_t> interfaces;

  static auto parse_from_bytes(std::span<std::byte> bytes)
      -> std::expected<classfile, classfile_reader_error>;
};
} // namespace pillar

#endif
