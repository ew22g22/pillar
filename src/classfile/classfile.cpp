#include <algorithm>
#include <bit>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <numeric>
#include <ranges>
#include <type_traits>
#include <utility>

#include "classfile.hpp"

template <typename F, typename Idx>
concept ReadManyTransformable = requires (F transform, Idx i) {
  {
    transform(i)
  } -> std::same_as<std::expected<typename decltype(transform(i))::value_type,
                                  pillar::classfile_reader_error>>;
};

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

  auto error_invalid_cp_tag(this classfile_reader const &self)
      -> std::unexpected<pillar::classfile_reader_error> {
    return self.error_with_reason_and_byte_index<
        pillar::classfile_reader_error_reason::INVALID_CP_TAG>();
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

  template <ReadManyTransformable<int> F>
  auto read_many(this classfile_reader &self, int count, F transform) {
    using Expected = std::invoke_result_t<F, int>;
    using T = typename Expected::value_type;

    auto const r = std::views::iota(0, static_cast<int>(count))
                 | std::views::transform(
                       [&transform](auto const i) { return transform(i); });

    return std::ranges::fold_left(
        r,
        std::expected<std::vector<T>, pillar::classfile_reader_error>{
            std::in_place_t{}},
        [](auto accum, auto const &right) {
          return accum.and_then([&right, accum = std::move(accum)](auto &left) {
            return right.and_then([&left, accum = std::move(accum)](auto &r) {
              left.emplace_back(r);
              return accum;
            });
          });
        });
  }

  auto read_magic(this classfile_reader &self)
      -> std::expected<void, pillar::classfile_reader_error> {
    return self.read_unsigned<pillar::u4_t>().and_then(
        [&self](auto const magic)
            -> std::expected<void, pillar::classfile_reader_error> {
          if (magic != static_cast<pillar::u4_t>(0xCAFEBABE)) {
            return classfile_reader::error_invalid_magic_number();
          }
          return std::expected<void, pillar::classfile_reader_error>{};
        });
  }

  auto read_versions(this classfile_reader &self)
      -> std::expected<std::pair<pillar::u2_t, pillar::u2_t>,
                       pillar::classfile_reader_error> {
    return self.read_unsigned<pillar::u2_t>().and_then(
        [&self](auto const minor_version) {
          return self.read_unsigned<pillar::u2_t>().transform(
              [minor_version](auto const major_version) {
                return std::pair{minor_version, major_version};
              });
        });
  }

  auto read_constant_pool_count(this classfile_reader &self)
      -> std::expected<pillar::u2_t, pillar::classfile_reader_error> {
    return self.read_unsigned<pillar::u2_t>();
  };

  auto read_cp_info(this classfile_reader &self, pillar::u1_t tag)
      -> std::expected<pillar::cp_info_t, pillar::classfile_reader_error> {
    static auto const read_single
        = [&self]<pillar::cp_info_tag Tag, typename T>() {
            return self.read_unsigned<T>().transform([](auto const fst) {
              return pillar::make_cp_info_t<Tag>(fst);
            });
          };

    static auto const read_double
        = [&self]<pillar::cp_info_tag Tag, typename T>() {
            return self.read_unsigned<T>()
                .and_then([&self](auto const fst) {
                  return self.read_unsigned<T>().transform(
                      [fst](auto const snd) { return std::pair{fst, snd}; });
                })
                .transform([&self](auto const pair) {
                  auto const [fst, snd] = pair;
                  return pillar::make_cp_info_t<Tag>(fst, snd);
                });
          };

    switch (static_cast<pillar::cp_info_tag>(tag)) {
    case pillar::cp_info_tag::CLASS:
      return read_single
          .template operator()<pillar::cp_info_tag::CLASS, pillar::u2_t>();

    case pillar::cp_info_tag::FIELDREF:
      return read_double
          .template operator()<pillar::cp_info_tag::FIELDREF, pillar::u2_t>();

    case pillar::cp_info_tag::METHODREF:
      return read_double
          .template operator()<pillar::cp_info_tag::METHODREF, pillar::u2_t>();

    case pillar::cp_info_tag::INTERFACE_METHODREF:
      return read_double.template
      operator()<pillar::cp_info_tag::INTERFACE_METHODREF, pillar::u2_t>();

    case pillar::cp_info_tag::STRING:
      return read_single
          .template operator()<pillar::cp_info_tag::STRING, pillar::u2_t>();

    case pillar::cp_info_tag::INTEGER:
      return read_single
          .template operator()<pillar::cp_info_tag::INTEGER, pillar::u4_t>();

    case pillar::cp_info_tag::FLOAT:
      return read_single
          .template operator()<pillar::cp_info_tag::FLOAT, pillar::u4_t>();

    case pillar::cp_info_tag::LONG:
      return read_double
          .template operator()<pillar::cp_info_tag::LONG, pillar::u4_t>();

    case pillar::cp_info_tag::DOUBLE:
      return read_double
          .template operator()<pillar::cp_info_tag::DOUBLE, pillar::u4_t>();

    case pillar::cp_info_tag::NAME_AND_TYPE:
      return read_double.template
      operator()<pillar::cp_info_tag::NAME_AND_TYPE, pillar::u2_t>();

    case pillar::cp_info_tag::UTF8:
      /* TODO */
      break;

    case pillar::cp_info_tag::METHOD_HANDLE:
      return self.read_unsigned<pillar::u1_t>()
          .and_then([&self](auto const fst) {
            return self.read_unsigned<pillar::u2_t>().transform(
                [fst](auto const snd) { return std::pair{fst, snd}; });
          })
          .transform([&self](auto const pair) {
            auto const [fst, snd] = pair;
            return pillar::make_cp_info_t<pillar::cp_info_tag::METHOD_HANDLE>(
                fst, snd);
          });

    case pillar::cp_info_tag::METHOD_TYPE:
      return read_single.template
      operator()<pillar::cp_info_tag::METHOD_TYPE, pillar::u2_t>();

    case pillar::cp_info_tag::DYNAMIC:
      return read_double
          .template operator()<pillar::cp_info_tag::DYNAMIC, pillar::u2_t>();

    case pillar::cp_info_tag::INVOKE_DYNAMIC:
      return read_double.template
      operator()<pillar::cp_info_tag::INVOKE_DYNAMIC, pillar::u2_t>();

    case pillar::cp_info_tag::MODULE:
      return read_single
          .template operator()<pillar::cp_info_tag::MODULE, pillar::u2_t>();

    case pillar::cp_info_tag::PACKAGE:
      return read_single
          .template operator()<pillar::cp_info_tag::PACKAGE, pillar::u2_t>();
    }
    return self.error_invalid_cp_tag();
  }

  auto read_constant_pool_data(this classfile_reader &self, pillar::u2_t count)
      -> std::expected<std::vector<pillar::cp_info_t>,
                       pillar::classfile_reader_error> {
    auto const r
        = std::views::iota(0, static_cast<int>(count))
        | std::views::transform([&self](auto const i) {
            return self.read_unsigned<pillar::u1_t>().and_then(
                [&self](auto const tag) { return self.read_cp_info(tag); });
          });

    return std::ranges::fold_left(
        r,
        std::expected<std::vector<pillar::cp_info_t>,
                      pillar::classfile_reader_error>{std::in_place_t{}},
        [](auto accum, auto const &right) {
          return accum.and_then([&right, accum = std::move(accum)](auto &left) {
            return right.and_then([&left, accum = std::move(accum)](auto &r) {
              left.emplace_back(r);
              return accum;
            });
          });
        });
  }

  auto read_class_info(this classfile_reader &self)
      -> std::expected<std::tuple<pillar::u2_t, pillar::u2_t, pillar::u2_t>,
                       pillar::classfile_reader_error> {
    return self.read_unsigned<pillar::u2_t>().and_then(
        [&self](auto const access_flags) {
          return self.read_unsigned<pillar::u2_t>().and_then(
              [&self, access_flags](auto const this_class) {
                return self.read_unsigned<pillar::u2_t>().transform(
                    [&self, access_flags, this_class](auto const super_class) {
                      return std::tuple{access_flags, this_class, super_class};
                    });
              });
        });
  }

  auto read_interfaces(this classfile_reader &self, pillar::u2_t count)
      -> std::expected<std::vector<pillar::u2_t>,
                       pillar::classfile_reader_error> {
    return self.read_many(static_cast<int>(count), [&self](auto) {
      return self.read_unsigned<pillar::u2_t>();
    });
  }
};

auto pillar::classfile::parse_from_bytes(std::span<std::byte> bytes)
    -> std::expected<classfile, classfile_reader_error> {
  auto file = classfile{};
  auto reader = classfile_reader{};

  return reader.read_magic()
      .and_then([&reader]() { return reader.read_versions(); })
      .transform([&file](auto const pair) {
        auto const [minor_version, major_version] = pair;
        file.minor_version = minor_version;
        file.major_version = major_version;
      })
      .and_then([&reader]() { return reader.read_constant_pool_count(); })
      .and_then([&reader](auto const count) {
        return reader.read_constant_pool_data(count);
      })
      .transform([&file](auto &&data) {
        file.constant_pool_count = data.size();
        file.constant_pool = std::move(data);
      })
      .and_then([&reader]() { return reader.read_class_info(); })
      .transform([&file](auto const tup) {
        auto const [access_flags, this_class, super_class] = tup;
        file.access_flags = access_flags;
        file.this_class = this_class;
        file.super_class = super_class;
      })
      .and_then([&reader]() { return reader.read_unsigned<pillar::u2_t>(); })
      .and_then(
          [&reader](auto const count) { return reader.read_interfaces(count); })
      .transform([&file](auto &&ifaces) {
        file.interfaces_count = ifaces.size();
        file.interfaces = std::move(ifaces);
      })
      .transform([file]() { return file; });
}
