#ifndef PILLAR_CLASSFILE_CLASSFILE_HPP
#define PILLAR_CLASSFILE_CLASSFILE_HPP

#include <cstddef>
#include <expected>
#include <span>

#include "cp_reader.hpp"

namespace pillar {
struct classfile {
  u4_t magic = 0xCAFEBABE;
  u2_t minor_version;
  u2_t major_version;
  u2_t constant_pool_count;
  std::vector<cp_info_t> constant_pool; /* [constant_pool_count] */
  u2_t access_flags;                    /* TODO: Make this an enum struct */
  u2_t this_class;
  u2_t super_class;
  u2_t interfaces_count;
  std::vector<u2_t> interfaces; /* [interfaces_count] */
  u2_t fields_count;

  u2_t attributes_count;
  std::vector<attribute_info_t> attributes; /* [attributes_count] */

  static auto constexpr parse_from_bytes(std::span<const std::byte> bytes)
      -> std::expected<classfile, classfile_reader_error> {
    auto reader = classfile_reader{.bytes = bytes};
    return reader
        .read<classfile_reader::value_wrapper<static_cast<u4_t>(0xCAFEBABE)>,
              u2_t,
              u2_t,
              classfile_reader::array_wrapper<cp_info_t, u2_t>,
              u2_t,
              u2_t,
              u2_t,
              classfile_reader::array_wrapper<u2_t, u2_t>>()
        .transform([](auto &&p) {
          auto &&[magic,
                  minor_version,
                  major_version,
                  cp,
                  access_flags,
                  this_class,
                  super_class,
                  ifaces]
              = p;
          return classfile{.magic = magic,
                           .minor_version = minor_version,
                           .major_version = major_version,
                           .constant_pool_count = static_cast<u2_t>(cp.size()),
                           .constant_pool = std::move(cp),
                           .access_flags = access_flags,
                           .this_class = this_class,
                           .super_class = super_class,
                           .interfaces_count = static_cast<u2_t>(ifaces.size()),
                           .interfaces = std::move(ifaces)};
        });
  }
};
} // namespace pillar

#endif
