#ifndef PILLAR_CLASSFILE_CP_READER_HPP
#define PILLAR_CLASSFILE_CP_READER_HPP

#include <algorithm>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <expected>
#include <format>
#include <functional>
#include <limits>
#include <numeric>
#include <print>
#include <ranges>
#include <span>
#include <type_traits>
#include <utility>

#include "attribute_info.hpp"
#include "classfile_types.hpp"
#include "cp_info.hpp"

namespace pillar {
enum struct classfile_reader_error_reason {
  NOT_ENOUGH_BYTES,
  INVALID_EXPECTED_VALUE,
  INVALID_CP_TAG,
};

struct classfile_reader_error {
  inline static auto constexpr NO_BYTE_INDEX
      = std::numeric_limits<std::span<std::byte>::size_type>::max();

  classfile_reader_error_reason reason;
  std::span<std::byte>::size_type byte_index;
};

struct classfile_reader {
  template <typename T, typename N>
  struct array_wrapper {};

  template <auto V>
  struct value_wrapper {};

  std::span<const std::byte> bytes{};
  std::span<const std::byte>::size_type byte_index{};

  template <classfile_reader_error_reason Reason>
  auto constexpr error_with_reason_and_byte_index(
      this classfile_reader const &self) {
    return std::unexpected{classfile_reader_error{
        .reason = Reason,
        .byte_index = self.byte_index,
    }};
  }

  auto constexpr error_not_enough_bytes(this classfile_reader const &self)
      -> std::unexpected<classfile_reader_error> {
    return self.error_with_reason_and_byte_index<
        classfile_reader_error_reason::NOT_ENOUGH_BYTES>();
  }

  auto constexpr error_invalid_expected_value(this classfile_reader const &self)
      -> std::unexpected<classfile_reader_error> {
    return self.error_with_reason_and_byte_index<
        classfile_reader_error_reason::INVALID_EXPECTED_VALUE>();
  }

  auto constexpr error_invalid_cp_tag(this classfile_reader const &self)
      -> std::unexpected<classfile_reader_error> {
    return self.error_with_reason_and_byte_index<
        classfile_reader_error_reason::INVALID_CP_TAG>();
  }

  template <typename T>
  auto constexpr read_unsigned(this classfile_reader &self)
      -> std::expected<T, classfile_reader_error> {
    static auto constexpr sz
        = static_cast<std::span<std::byte>::size_type>(sizeof(T));

    if (self.bytes.size() - self.byte_index < sz) {
      return self.error_not_enough_bytes();
    }

    auto const n = [&self]() {
      auto arr = std::array<std::byte, sz>{};
      auto const bys = self.bytes.subspan(self.byte_index, sz);
      std::ranges::copy(bys, std::begin(arr));
      return std::byteswap(std::bit_cast<T>(arr));
    }();

    self.byte_index += sz;
    return std::expected<T, classfile_reader_error>{n};
  }

  template <typename T>
  auto constexpr read_many(this classfile_reader &self, int count) {
    auto const r
        = std::views::iota(0, count)
        | std::views::transform([&self]([[maybe_unused]] auto const i) {
            return self.read<T>();
          });

    /* TODO: LUGLY clean up */
    return std::ranges::fold_left(
        r,
        std::expected<std::vector<T>, classfile_reader_error>{std::in_place,
                                                              std::vector<T>{}},
        [](auto accum, auto const &right)
            -> std::expected<std::vector<T>, classfile_reader_error> {
          if (!accum) {
            return accum;
          }
          if (!right) {
            return std::unexpected<classfile_reader_error>{right.error()};
          }
          accum->push_back(*right);
          return accum;
        });
  }

  template <typename... Ts>
  auto constexpr read(this classfile_reader &self) -> decltype(auto) {
    if constexpr (sizeof...(Ts) == 1) {
      using T = std::tuple_element_t<0, std::tuple<Ts...>>;
      return read_entry<T>::read(self);
    } else {
      return read_entries<Ts...>::read(self);
    }
  }

private:
  template <typename T>
  struct read_entry;

  template <>
  struct read_entry<u1_t> {
    static auto constexpr read(classfile_reader &reader)
        -> std::expected<u1_t, classfile_reader_error> {
      return reader.read_unsigned<u1_t>();
    }
  };

  template <>
  struct read_entry<std::byte> {
    static auto constexpr read(classfile_reader &reader)
        -> std::expected<std::byte, classfile_reader_error> {
      return reader.read_unsigned<u1_t>().transform(
          [](auto const by) { return static_cast<std::byte>(by); });
    }
  };

  template <>
  struct read_entry<u2_t> {
    static auto constexpr read(classfile_reader &reader)
        -> std::expected<u2_t, classfile_reader_error> {
      return reader.read_unsigned<u2_t>();
    }
  };

  template <>
  struct read_entry<u4_t> {
    static auto constexpr read(classfile_reader &reader)
        -> std::expected<u4_t, classfile_reader_error> {
      return reader.read_unsigned<u4_t>();
    }
  };

  template <>
  struct read_entry<cp_info<cp_info_tag::CLASS>> {
    static auto constexpr read(classfile_reader &reader)
        -> std::expected<cp_info<cp_info_tag::CLASS>, classfile_reader_error> {
      return reader.read<u2_t>().transform([](auto const name_index) {
        return cp_info<cp_info_tag::CLASS>{.name_index = name_index};
      });
    }
  };

  template <>
  struct read_entry<cp_info<cp_info_tag::FIELDREF>> {
    static auto constexpr read(classfile_reader &reader)
        -> std::expected<cp_info<cp_info_tag::FIELDREF>,
                         classfile_reader_error> {
      return reader.read<u2_t, u2_t>().transform([](auto const t) {
        auto const [class_index, name_and_type_index] = t;
        return cp_info<cp_info_tag::FIELDREF>{.class_index
                                              = name_and_type_index};
      });
    }
  };

  template <>
  struct read_entry<cp_info<cp_info_tag::METHODREF>> {
    static auto constexpr read(classfile_reader &reader)
        -> std::expected<cp_info<cp_info_tag::METHODREF>,
                         classfile_reader_error> {
      return reader.read<u2_t, u2_t>().transform([](auto const t) {
        auto const [class_index, name_and_type_index] = t;
        return cp_info<cp_info_tag::METHODREF>{.class_index
                                               = name_and_type_index};
      });
    }
  };

  template <>
  struct read_entry<cp_info<cp_info_tag::INTERFACE_METHODREF>> {
    static auto constexpr read(classfile_reader &reader)
        -> std::expected<cp_info<cp_info_tag::INTERFACE_METHODREF>,
                         classfile_reader_error> {
      return reader.read<u2_t, u2_t>().transform([](auto const t) {
        auto const [class_index, name_and_type_index] = t;
        return cp_info<cp_info_tag::INTERFACE_METHODREF>{.class_index
                                                         = name_and_type_index};
      });
    }
  };

  template <>
  struct read_entry<cp_info<cp_info_tag::STRING>> {
    static auto constexpr read(classfile_reader &reader)
        -> std::expected<cp_info<cp_info_tag::STRING>, classfile_reader_error> {
      return reader.read<u2_t>().transform([](auto const string_index) {
        return cp_info<cp_info_tag::STRING>{.string_index = string_index};
      });
    }
  };

  template <>
  struct read_entry<cp_info<cp_info_tag::INTEGER>> {
    static auto constexpr read(classfile_reader &reader)
        -> std::expected<cp_info<cp_info_tag::INTEGER>,
                         classfile_reader_error> {
      return reader.read<u4_t>().transform([](auto const bytes) {
        return cp_info<cp_info_tag::INTEGER>{.bytes = bytes};
      });
    }
  };

  template <>
  struct read_entry<cp_info<cp_info_tag::FLOAT>> {
    static auto constexpr read(classfile_reader &reader)
        -> std::expected<cp_info<cp_info_tag::FLOAT>, classfile_reader_error> {
      return reader.read<u4_t>().transform([](auto const bytes) {
        return cp_info<cp_info_tag::FLOAT>{.bytes = bytes};
      });
    }
  };

  template <>
  struct read_entry<cp_info<cp_info_tag::LONG>> {
    static auto constexpr read(classfile_reader &reader)
        -> std::expected<cp_info<cp_info_tag::LONG>, classfile_reader_error> {
      return reader.read<u4_t, u4_t>().transform([](auto const t) {
        auto const [high_bytes, low_bytes] = t;
        return cp_info<cp_info_tag::LONG>{.high_bytes = high_bytes,
                                          .low_bytes = low_bytes};
      });
    }
  };

  template <>
  struct read_entry<cp_info<cp_info_tag::DOUBLE>> {
    static auto constexpr read(classfile_reader &reader)
        -> std::expected<cp_info<cp_info_tag::DOUBLE>, classfile_reader_error> {
      return reader.read<u4_t, u4_t>().transform([](auto const t) {
        auto const [high_bytes, low_bytes] = t;
        return cp_info<cp_info_tag::DOUBLE>{.high_bytes = high_bytes,
                                            .low_bytes = low_bytes};
      });
    }
  };

  template <>
  struct read_entry<cp_info<cp_info_tag::NAME_AND_TYPE>> {
    static auto constexpr read(classfile_reader &reader)
        -> std::expected<cp_info<cp_info_tag::NAME_AND_TYPE>,
                         classfile_reader_error> {
      return reader.read<u2_t, u2_t>().transform([](auto const t) {
        auto const [name_index, descriptor_index] = t;
        return cp_info<cp_info_tag::NAME_AND_TYPE>{
            .name_index = name_index, .descriptor_index = descriptor_index};
      });
    }
  };

  template <>
  struct read_entry<cp_info<cp_info_tag::UTF8>> {
    static auto constexpr read(classfile_reader &reader)
        -> std::expected<cp_info<cp_info_tag::UTF8>, classfile_reader_error> {
      return reader.read<array_wrapper<std::byte, u2_t>>().transform(
          [](auto &&vec) {
            return cp_info<cp_info_tag::UTF8>{
                .length = static_cast<u2_t>(vec.size()),
                .bytes = std::forward<decltype(vec)>(vec)};
          });
    }
  };

  template <>
  struct read_entry<cp_info<cp_info_tag::METHOD_HANDLE>> {
    static auto constexpr read(classfile_reader &reader)
        -> std::expected<cp_info<cp_info_tag::METHOD_HANDLE>,
                         classfile_reader_error> {
      return reader.read<u2_t, u2_t>().transform([](auto const t) {
        auto const [reference_kind, reference_index] = t;
        return cp_info<cp_info_tag::METHOD_HANDLE>{
            .reference_kind = reference_kind,
            .reference_index = reference_index};
      });
    }
  };

  template <>
  struct read_entry<cp_info<cp_info_tag::METHOD_TYPE>> {
    static auto constexpr read(classfile_reader &reader)
        -> std::expected<cp_info<cp_info_tag::METHOD_TYPE>,
                         classfile_reader_error> {
      return reader.read<u2_t>().transform([](auto const descriptor_index) {
        return cp_info<cp_info_tag::METHOD_TYPE>{.descriptor_index
                                                 = descriptor_index};
      });
    }
  };

  template <>
  struct read_entry<cp_info<cp_info_tag::DYNAMIC>> {
    static auto constexpr read(classfile_reader &reader)
        -> std::expected<cp_info<cp_info_tag::DYNAMIC>,
                         classfile_reader_error> {
      return reader.read<u2_t, u2_t>().transform([](auto const t) {
        auto const [bootstrap_method_attr_index, name_and_type_index] = t;
        return cp_info<cp_info_tag::DYNAMIC>{
            .bootstrap_method_attr_index = bootstrap_method_attr_index,
            .name_and_type_index = name_and_type_index};
      });
    }
  };

  template <>
  struct read_entry<cp_info<cp_info_tag::INVOKE_DYNAMIC>> {
    static auto constexpr read(classfile_reader &reader)
        -> std::expected<cp_info<cp_info_tag::INVOKE_DYNAMIC>,
                         classfile_reader_error> {
      return reader.read<u2_t, u2_t>().transform([](auto const t) {
        auto const [bootstrap_method_attr_index, name_and_type_index] = t;
        return cp_info<cp_info_tag::INVOKE_DYNAMIC>{
            .bootstrap_method_attr_index = bootstrap_method_attr_index,
            .name_and_type_index = name_and_type_index};
      });
    }
  };

  template <>
  struct read_entry<cp_info<cp_info_tag::MODULE>> {
    static auto constexpr read(classfile_reader &reader)
        -> std::expected<cp_info<cp_info_tag::MODULE>, classfile_reader_error> {
      return reader.read<u2_t>().transform([](auto const name_index) {
        return cp_info<cp_info_tag::MODULE>{.name_index = name_index};
      });
    }
  };

  template <>
  struct read_entry<cp_info<cp_info_tag::PACKAGE>> {
    static auto constexpr read(classfile_reader &reader)
        -> std::expected<cp_info<cp_info_tag::PACKAGE>,
                         classfile_reader_error> {
      return reader.read<u2_t>().transform([](auto const name_index) {
        return cp_info<cp_info_tag::PACKAGE>{.name_index = name_index};
      });
    }
  };

  template <>
  struct read_entry<cp_info_t> {
    static auto constexpr read(classfile_reader &reader)
        -> std::expected<cp_info_t, classfile_reader_error> {
      return reader.read<u1_t>().and_then([&reader](auto const tag) {
        switch (static_cast<cp_info_tag>(tag)) {
        case cp_info_tag::CLASS:
          return reader.read<cp_info<cp_info_tag::CLASS>>().transform(
              [](auto const v) { return cp_info_t{v}; });
        case cp_info_tag::FIELDREF:
          return reader.read<cp_info<cp_info_tag::FIELDREF>>().transform(
              [](auto const v) { return cp_info_t{v}; });
        case cp_info_tag::METHODREF:
          return reader.read<cp_info<cp_info_tag::METHODREF>>().transform(
              [](auto const v) { return cp_info_t{v}; });
        case cp_info_tag::INTERFACE_METHODREF:
          return reader.read<cp_info<cp_info_tag::INTERFACE_METHODREF>>()
              .transform([](auto const v) { return cp_info_t{v}; });
        case cp_info_tag::STRING:
          return reader.read<cp_info<cp_info_tag::STRING>>().transform(
              [](auto const v) { return cp_info_t{v}; });
        case cp_info_tag::INTEGER:
          return reader.read<cp_info<cp_info_tag::INTEGER>>().transform(
              [](auto const v) -> cp_info_t { return v; });
        case cp_info_tag::FLOAT:
          return reader.read<cp_info<cp_info_tag::FLOAT>>().transform(
              [](auto const v) { return cp_info_t{v}; });
        case cp_info_tag::LONG:
          return reader.read<cp_info<cp_info_tag::LONG>>().transform(
              [](auto const v) { return cp_info_t{v}; });
        case cp_info_tag::DOUBLE:
          return reader.read<cp_info<cp_info_tag::DOUBLE>>().transform(
              [](auto const v) { return cp_info_t{v}; });
        case cp_info_tag::NAME_AND_TYPE:
          return reader.read<cp_info<cp_info_tag::NAME_AND_TYPE>>().transform(
              [](auto const v) { return cp_info_t{v}; });
        case cp_info_tag::UTF8:
          return reader.read<cp_info<cp_info_tag::UTF8>>().transform(
              [](auto const v) { return cp_info_t{v}; });
        case cp_info_tag::METHOD_HANDLE:
          return reader.read<cp_info<cp_info_tag::METHOD_HANDLE>>().transform(
              [](auto const v) { return cp_info_t{v}; });
        case cp_info_tag::METHOD_TYPE:
          return reader.read<cp_info<cp_info_tag::METHOD_TYPE>>().transform(
              [](auto const v) { return cp_info_t{v}; });
        case cp_info_tag::DYNAMIC:
          return reader.read<cp_info<cp_info_tag::DYNAMIC>>().transform(
              [](auto const v) { return cp_info_t{v}; });
        case cp_info_tag::INVOKE_DYNAMIC:
          return reader.read<cp_info<cp_info_tag::INVOKE_DYNAMIC>>().transform(
              [](auto const v) { return cp_info_t{v}; });
        case cp_info_tag::MODULE:
          return reader.read<cp_info<cp_info_tag::MODULE>>().transform(
              [](auto const v) { return cp_info_t{v}; });
        case cp_info_tag::PACKAGE:
          return reader.read<cp_info<cp_info_tag::PACKAGE>>().transform(
              [](auto const v) { return cp_info_t{v}; });
        }
      });
    }
  };

  template <auto V>
  struct read_entry<value_wrapper<V>> {
    static auto constexpr read(classfile_reader &reader) {
      using T = decltype(V);
      return reader.read<T>().and_then(
          [&reader](auto const &v) -> std::expected<T, classfile_reader_error> {
            if (v != V) {
              return reader.error_invalid_expected_value();
            }
            return std::expected<T, classfile_reader_error>{v};
          });
    }
  };

  template <typename T>
  struct read_entry<array_wrapper<T, u1_t>> {
    static auto constexpr read(classfile_reader &reader) {
      return reader.read<u1_t>().and_then([&reader](auto const n) {
        return reader.read_many<T>(static_cast<int>(n));
      });
    }
  };

  template <typename T>
  struct read_entry<array_wrapper<T, u2_t>> {
    static auto constexpr read(classfile_reader &reader) -> std::expected<
        std::vector<typename decltype(read_entry<T>::read(reader))::value_type>,
        classfile_reader_error> {
      return reader.read<u2_t>().and_then([&reader](auto const n) {
        return reader.read_many<T>(static_cast<int>(n));
      });
    }
  };

  template <typename... Ts>
  struct read_entries;

  template <>
  struct read_entries<> {
    static auto constexpr read([[maybe_unused]] classfile_reader &reader) {
      return std::expected<std::tuple<>, classfile_reader_error>{
          std::in_place_t{}};
    }
  };

  template <typename T>
  struct read_entries<T> {
    static auto constexpr read(classfile_reader &reader) {
      return read_entry<T>::read(reader).transform([](auto &&v) {
        return std::make_tuple(std::forward<decltype(v)>(v));
      });
    }
  };

  template <typename T, typename... Remainder>
  struct read_entries<T, Remainder...> {
    static auto constexpr read(classfile_reader &reader) -> std::expected<
        std::tuple<typename decltype(read_entry<T>::read(reader))::value_type,
                   typename decltype(read_entry<Remainder>::read(
                       reader))::value_type...>,
        classfile_reader_error> {
      return read_entries<T>::read(reader).and_then([&reader](auto &&fst) {
        return read_entries<Remainder...>::read(reader).transform(
            [fst = std::forward<decltype(fst)>(fst)](auto &&rest) mutable {
              return std::tuple_cat(std::move(fst),
                                    std::forward<decltype(rest)>(rest));
            });
      });
    }
  };
};
} // namespace pillar

namespace std {
template <>
struct formatter<pillar::classfile_reader_error, char> {
  constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const pillar::classfile_reader_error &error,
              FormatContext &ctx) const {
    switch (error.reason) {
    case pillar::classfile_reader_error_reason::INVALID_CP_TAG:
      return std::format_to(
          ctx.out(), "Invalid CP tag found at index: {}", error.byte_index);
    case pillar::classfile_reader_error_reason::INVALID_EXPECTED_VALUE:
      return std::format_to(ctx.out(),
                            "Found incorrect expected value at index: {}",
                            error.byte_index);
    case pillar::classfile_reader_error_reason::NOT_ENOUGH_BYTES:
      return std::format_to(ctx.out(),
                            "Not enough bytes to continue at index: {}",
                            error.byte_index);
    }
  }
};
} // namespace std

#endif
