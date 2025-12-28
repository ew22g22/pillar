#include <bit>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <utility>

#include "classfile.hpp"

struct classfile_reader {
  std::span<std::byte> bytes{};
  std::span<std::byte>::size_type byte_index{};

  template <pillar::classfile_reader_error_reason Reason>
  auto error_with_reason_and_byte_index(this classfile_reader const &self) {
    return std::unexpected{pillar::classfile_reader_error{
        .reason = Reason,
        .byte_index = self.byte_index,
    }};
  }

  auto error_not_enough_bytes(this classfile_reader const &self)
      -> std::unexpected<pillar::classfile_reader_error> {
    return self.error_with_reason_and_byte_index<
        pillar::classfile_reader_error_reason::NOT_ENOUGH_BYTES>();
  }

  static auto error_invalid_magic_number()
      -> std::unexpected<pillar::classfile_reader_error> {
    return std::unexpected{pillar::classfile_reader_error{
        .reason = pillar::classfile_reader_error_reason::INVALID_MAGIC_NUMBER,
        .byte_index = pillar::classfile_reader_error::NO_BYTE_INDEX,
    }};
  }

  template <typename T>
  auto read_unsigned(this classfile_reader &self)
      -> std::expected<T, pillar::classfile_reader_error> {
    static auto constexpr sz
        = static_cast<std::span<std::byte>::size_type>(sizeof(T));

    if (self.bytes.size() < sz) {
      return self.error_not_enough_bytes();
    }

    auto const n = [&self](std::span<std::byte> bytes) {
      auto n = T{};
      auto const bys = self.bytes.subspan(self.byte_index, sz);
      std::memcpy(&n, bys.data(), sizeof(n));
      return std::byteswap(n);
    }(self.bytes);

    self.byte_index += sz;
    return std::expected<T, pillar::classfile_reader_error>{n};
  }

  auto read_magic(this classfile_reader &self)
      -> std::expected<void, pillar::classfile_reader_error> {
    return self.read_unsigned<pillar::classfile::u4_t>().and_then(
        [&self](auto const magic)
            -> std::expected<void, pillar::classfile_reader_error> {
          if (magic != static_cast<pillar::classfile::u4_t>(0xCAFEBABE)) {
            return classfile_reader::error_invalid_magic_number();
          }
          return std::expected<void, pillar::classfile_reader_error>{};
        });
  }

  auto read_versions(this classfile_reader &self) -> std::expected<
      std::pair<pillar::classfile::u2_t, pillar::classfile::u2_t>,
      pillar::classfile_reader_error> {
    return self.read_unsigned<pillar::classfile::u2_t>().and_then(
        [&self](auto minor_version) {
          return self.read_unsigned<pillar::classfile::u2_t>().transform(
              [minor_version](auto major_version) {
                return std::pair{minor_version, major_version};
              });
        });
  }
};

auto pillar::classfile::parse_from_bytes(std::span<std::byte> bytes)
    -> std::expected<classfile, classfile_reader_error> {
  auto file = classfile{};
  auto reader = classfile_reader{};

  return reader.read_unsigned<classfile::u4_t>()
      .and_then([&reader](auto) { return reader.read_magic(); })
      .and_then([&reader]() { return reader.read_versions(); })
      .and_then([&reader, &file](auto const pair) {
        auto const [minor_version, major_version] = pair;
        file.minor_version = minor_version;
        file.major_version = major_version;
        return std::expected<void, classfile_reader_error>{};
      })
      .transform([file]() { return file; });
}
