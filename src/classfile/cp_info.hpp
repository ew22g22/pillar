#ifndef PILLAR_CLASSFILE_CP_INFO_HPP
#define PILLAR_CLASSFILE_CP_INFO_HPP

#include <utility>
#include <variant>
#include <vector>

#include "classfile_types.hpp"

namespace pillar {
enum struct cp_info_tag : u1_t {
  CLASS = 7,
  FIELDREF = 9,
  METHODREF = 10,
  INTERFACE_METHODREF = 11,
  STRING = 8,
  INTEGER = 3,
  FLOAT = 4,
  LONG = 5,
  DOUBLE = 6,
  NAME_AND_TYPE = 12,
  UTF8 = 1,
  METHOD_HANDLE = 15,
  METHOD_TYPE = 16,
  DYNAMIC = 17,
  INVOKE_DYNAMIC = 18,
  MODULE = 19,
  PACKAGE = 20,
};

template <cp_info_tag Tag>
struct cp_info;

template <>
struct cp_info<cp_info_tag::CLASS> {
  u2_t name_index;
};

template <>
struct cp_info<cp_info_tag::FIELDREF> {
  u2_t class_index;
  u2_t name_and_type_index;
};

template <>
struct cp_info<cp_info_tag::METHODREF> {
  u2_t class_index;
  u2_t name_and_type_index;
};

template <>
struct cp_info<cp_info_tag::INTERFACE_METHODREF> {
  u2_t class_index;
  u2_t name_and_type_index;
};

template <>
struct cp_info<cp_info_tag::STRING> {
  u2_t string_index;
};

template <>
struct cp_info<cp_info_tag::INTEGER> {
  u4_t bytes;
};

template <>
struct cp_info<cp_info_tag::FLOAT> {
  u4_t bytes;
};

template <>
struct cp_info<cp_info_tag::LONG> {
  u4_t high_bytes;
  u4_t low_bytes;
};

template <>
struct cp_info<cp_info_tag::DOUBLE> {
  u4_t high_bytes;
  u4_t low_bytes;
};

template <>
struct cp_info<cp_info_tag::NAME_AND_TYPE> {
  u2_t name_index;
  u2_t descriptor_index;
};

template <>
struct cp_info<cp_info_tag::UTF8> {
  u2_t length;
  std::vector<std::byte> bytes;
};

template <>
struct cp_info<cp_info_tag::METHOD_HANDLE> {
  u2_t reference_kind;
  u2_t reference_index;
};

template <>
struct cp_info<cp_info_tag::METHOD_TYPE> {
  u2_t descriptor_index;
};

template <>
struct cp_info<cp_info_tag::DYNAMIC> {
  u2_t bootstrap_method_attr_index;
  u2_t name_and_type_index;
};

template <>
struct cp_info<cp_info_tag::INVOKE_DYNAMIC> {
  u2_t bootstrap_method_attr_index;
  u2_t name_and_type_index;
};

template <>
struct cp_info<cp_info_tag::MODULE> {
  u2_t name_index;
};

template <>
struct cp_info<cp_info_tag::PACKAGE> {
  u2_t name_index;
};

template <cp_info_tag... Tags>
using cp_infos_t = std::variant<cp_info<Tags>...>;
using cp_info_t = cp_infos_t<
    cp_info_tag::CLASS, cp_info_tag::FIELDREF, cp_info_tag::METHODREF,
    cp_info_tag::INTERFACE_METHODREF, cp_info_tag::STRING, cp_info_tag::INTEGER,
    cp_info_tag::FLOAT, cp_info_tag::LONG, cp_info_tag::DOUBLE,
    cp_info_tag::NAME_AND_TYPE, cp_info_tag::UTF8, cp_info_tag::METHOD_HANDLE,
    cp_info_tag::METHOD_TYPE, cp_info_tag::DYNAMIC, cp_info_tag::INVOKE_DYNAMIC,
    cp_info_tag::MODULE, cp_info_tag::PACKAGE>;

template <cp_info_tag Tag, typename... Args>
inline auto make_cp_info_t(Args &&...args) -> cp_info_t {
  return cp_info_t{cp_info<Tag>{std::forward<Args>(args)...}};
}

template <typename T>
struct cp_info_tag_of {};

template <cp_info_tag Tag>
struct cp_info_tag_of<cp_info<Tag>> {
  inline static auto constexpr tag = Tag;
};

template <typename T>
inline auto constexpr cp_info_tag_of_v = cp_info_tag_of<T>::tag;

} // namespace pillar

#endif
