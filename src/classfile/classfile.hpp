#ifndef PILLAR_CLASSFILE_CLASSFILE_HPP
#define PILLAR_CLASSFILE_CLASSFILE_HPP

#include <cstddef>
#include <expected>
#include <span>

#include "classfile_types.hpp"

namespace pillar {
enum struct classfile_reader_error_reason {
  NOT_ENOUGH_BYTES,
  INVALID_MAGIC_NUMBER,
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

  static auto parse_from_bytes(std::span<std::byte> bytes)
      -> std::expected<classfile, classfile_reader_error>;
};
} // namespace pillar

#endif
