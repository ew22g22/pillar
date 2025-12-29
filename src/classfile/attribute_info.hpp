#ifndef PILLAR_CLASSFILE_ATTRIBUTE_INFO_HPP
#define PILLAR_CLASSFILE_ATTRIBUTE_INFO_HPP

#include <cstddef>
#include <memory>
#include <variant>
#include <vector>

#include "classfile_types.hpp"

namespace pillar {
/* Encoding attributes into the type system like this means that it won't be
 * possible to support on the fly (runtime) custom attribute detection, or at
 * least it wont be possible cleanly. This isnt a problem at the moment as the
 * spec states that unknown attributes must be silently ignored; however a
 * redesign might be required in future if this is a needed feature.
 */
enum struct attribute_info_type {
  CONSTANT_VALUE,
  CODE,
  STACK_MAP_TABLE,
  BOOTSTRAP_METHODS,
  NEST_HOST,
  NEST_MEMBERS,
  PERMITTED_SUBCLASSES,
  EXCEPTIONS,
  INNER_CLASSES,
  ENCLOSING_METHOD,
  SYNTHETIC,
  SIGNATURE,
  RECORD,
  SOURCE_FILE,
  LINE_NUMBER_TABLE,
  LOCAL_VARIABLE_TABLE,
  LOCAL_VARIABLE_TYPE_TABLE,
  SOURCE_DEBUG_EXTENSION,
  DEPRECATED,
  RUNTIME_VISIBLE_ANNOTATIONS,
  RUNTIME_INVISIBLE_ANNOTATIONS,
  RUNTIME_VISIBLE_PARAMETER_ANNOTATIONS,
  RUNTIME_INVISIBLE_PARAMETER_ANNOTATIONS,
  RUNTIME_VISIBLE_TYPE_ANNOTATIONS,
  RUNTIME_INVISIBLE_TYPE_ANNOTATIONS,
  ANNOTATION_DEFAULT,
  METHOD_PARAMETERS,
  MODULE,
  MODULE_PACKAGES,
  MODULE_MAIN_CLASS,
};

struct attribute_info_header {
  u2_t attribute_name_index;
  u4_t attribute_length;
};

template <attribute_info_type Type>
struct attribute_info;

using attribute_info_t
    = std::variant<attribute_info<attribute_info_type::CONSTANT_VALUE>,
                   attribute_info<attribute_info_type::CODE>>;

template <>
struct attribute_info<attribute_info_type::CONSTANT_VALUE>
    : attribute_info_header {
  u2_t constantvalue_index;
};

template <>
struct attribute_info<attribute_info_type::CODE> : attribute_info_header {
  u2_t max_stack;
  u2_t max_locals;
  u4_t code_length;
  std::vector<std::byte> code; /* [code_length] */

  u2_t exception_table_length;

  struct exception_table_entry {
    u2_t start_pc;
    u2_t end_pc;
    u2_t handler_pc;
    u2_t catch_type;
  };

  std::vector<exception_table_entry>
      exception_table; /* [exception_table_length] */
  u2_t attributes_count;
  std::vector<attribute_info_t> attributes; /* [attributes_count] */
};

enum struct verification_type_info_tag : u1_t {
  TOP = 0,
  INTEGER = 1,
  FLOAT = 2,
  NULL_ = 5,
  UNINITIALIZED_THIS = 6,
  OBJECT = 7,
  UNINITIALIZED = 8,
  LONG = 4,
  DOUBLE = 3,
};

template <verification_type_info_tag Tag>
struct verification_type_info;

template <>
struct verification_type_info<verification_type_info_tag::TOP> {};

template <>
struct verification_type_info<verification_type_info_tag::INTEGER> {};

template <>
struct verification_type_info<verification_type_info_tag::FLOAT> {};

template <>
struct verification_type_info<verification_type_info_tag::NULL_> {};

template <>
struct verification_type_info<verification_type_info_tag::UNINITIALIZED_THIS> {
};

template <>
struct verification_type_info<verification_type_info_tag::OBJECT> {
  u2_t cpool_index;
};

template <>
struct verification_type_info<verification_type_info_tag::UNINITIALIZED> {
  u2_t offset;
};

template <>
struct verification_type_info<verification_type_info_tag::LONG> {};

template <>
struct verification_type_info<verification_type_info_tag::DOUBLE> {};

template <verification_type_info_tag... Tags>
using verification_type_infos_t = std::variant<verification_type_info<Tags>...>;
using verification_type_info_t = verification_type_infos_t<
    verification_type_info_tag::TOP, verification_type_info_tag::INTEGER,
    verification_type_info_tag::FLOAT, verification_type_info_tag::NULL_,
    verification_type_info_tag::UNINITIALIZED_THIS,
    verification_type_info_tag::OBJECT,
    verification_type_info_tag::UNINITIALIZED, verification_type_info_tag::LONG,
    verification_type_info_tag::DOUBLE>;

/* These tags are a bit different. Whereas usually one value means an explicit
 * entry, the JVM assigns ranges of tags (eg 0-63) for certain elements, and
 * the number has some actual meaning, eg some offset. To handle this, we dont
 * assign values to enum entries and encode the type as an entry in memory for
 * use (u1_t frame_type).
 */
enum struct stack_map_frame_tag : u1_t {
  SAME,
  SAME_LOCALS_1_STACK_ITEM,
  SAME_LOCALS_1_STACK_ITEM_EXTENDED,
  CHOP,
  SAME_FRAME_EXTENDED,
  APPEND,
  FULL_FRAME,
};

template <stack_map_frame_tag Tag>
struct stack_map_frame;

template <>
struct stack_map_frame<stack_map_frame_tag::SAME> {
  u1_t frame_type; /* 0-63 */
};

template <>
struct stack_map_frame<stack_map_frame_tag::SAME_LOCALS_1_STACK_ITEM> {
  u1_t frame_type;                /* 64-127 */
  verification_type_info_t stack; /* [1] */
};

template <>
struct stack_map_frame<stack_map_frame_tag::SAME_LOCALS_1_STACK_ITEM_EXTENDED> {
  u1_t frame_type; /* 247 */
  u2_t offset_delta;
  verification_type_info_t stack; /* [1] */
};

template <>
struct stack_map_frame<stack_map_frame_tag::CHOP> {
  u1_t frame_type; /* 248-250 */
  u2_t offset_delta;
};

template <>
struct stack_map_frame<stack_map_frame_tag::SAME_FRAME_EXTENDED> {
  u1_t frame_type; /* 251 */
  u2_t offset_delta;
};

template <>
struct stack_map_frame<stack_map_frame_tag::APPEND> {
  u1_t frame_type; /* 252-254 */
  u2_t offset_delta;
  std::vector<verification_type_info_t> locals; /* [frame_type - 251] */
};

template <>
struct stack_map_frame<stack_map_frame_tag::FULL_FRAME> {
  u1_t frame_type; /* 255 */
  u2_t offset_delta;
  u2_t number_of_locals;
  std::vector<verification_type_info_t> locals; /* [number_of_locals] */
  u2_t number_of_stack_items;
  std::vector<verification_type_info_t> stack; /* [number_of_stack_items] */
};

template <stack_map_frame_tag... Tags>
using stack_map_frames_t = std::variant<stack_map_frame<Tags>...>;
using stack_map_frame_t = stack_map_frames_t<
    stack_map_frame_tag::SAME, stack_map_frame_tag::SAME_LOCALS_1_STACK_ITEM,
    stack_map_frame_tag::SAME_LOCALS_1_STACK_ITEM_EXTENDED,
    stack_map_frame_tag::CHOP, stack_map_frame_tag::SAME_FRAME_EXTENDED,
    stack_map_frame_tag::APPEND, stack_map_frame_tag::FULL_FRAME>;

template <>
struct attribute_info<attribute_info_type::STACK_MAP_TABLE>
    : attribute_info_header {
  u2_t number_of_entries;
  std::vector<stack_map_frame_t> entries; /* [number_of_entries] */
};

template <>
struct attribute_info<attribute_info_type::EXCEPTIONS> : attribute_info_header {
  u2_t number_of_exceptions;
  std::vector<u2_t> exception_index_table; /* [number_of_exceptions] */
};

enum struct inner_class_access_flags : u2_t {
  PUBLIC = ACC_PUBLIC,
  PRIVATE = ACC_PRIVATE,
  PROTECTED = ACC_PROTECTED,
  STATIC = ACC_STATIC,
  FINAL = ACC_FINAL,
  INTERFACE = ACC_INTERFACE,
  ABSTRACT = ACC_ABSTRACT,
  SYNTHETIC = ACC_SYNTHETIC,
  ANNOTATION = ACC_ANNOTATION,
  ENUM = ACC_ENUM,
};

template <>
struct attribute_info<attribute_info_type::INNER_CLASSES>
    : attribute_info_header {
  u2_t number_of_classes;

  struct classes_entry { /* Is there a better name? */
    u2_t inner_class_info_index;
    u2_t outer_class_info_index;
    u2_t inner_name_index;
    inner_class_access_flags inner_class_access_flags;
  };

  std::vector<classes_entry> classes; /* [number_of_classes] */
};

template <>
struct attribute_info<attribute_info_type::ENCLOSING_METHOD>
    : attribute_info_header {
  u2_t class_index;
  u2_t method_index;
};

template <>
struct attribute_info<attribute_info_type::SYNTHETIC> : attribute_info_header {
};

template <>
struct attribute_info<attribute_info_type::SIGNATURE> : attribute_info_header {
  u2_t signature_index;
};

template <>
struct attribute_info<attribute_info_type::SOURCE_FILE>
    : attribute_info_header {
  u2_t sourcefile_index;
};

template <>
struct attribute_info<attribute_info_type::SOURCE_DEBUG_EXTENSION>
    : attribute_info_header {
  std::vector<std::byte> debug_extension; /* [attribute_length] */
};

template <>
struct attribute_info<attribute_info_type::LINE_NUMBER_TABLE>
    : attribute_info_header {
  u2_t line_number_table_length;

  struct line_number_table_entry {
    u2_t start_pc;
    u2_t line_number;
  };

  std::vector<line_number_table_entry>
      line_number_table; /* [line_number_table_length] */
};

template <>
struct attribute_info<attribute_info_type::LOCAL_VARIABLE_TABLE>
    : attribute_info_header {
  u2_t local_variable_table_length;

  struct local_variable_table_entry {
    u2_t start_pc;
    u2_t length;
    u2_t name_index;
    u2_t descriptor_index;
    u2_t index;
  };

  std::vector<local_variable_table_entry>
      local_variable_table; /* [local_variable_table_length] */
};

template <>
struct attribute_info<attribute_info_type::LOCAL_VARIABLE_TYPE_TABLE>
    : attribute_info_header {
  u2_t local_variable_type_table_length;

  struct local_variable_type_table_entry {
    u2_t start_pc;
    u2_t length;
    u2_t name_index;
    u2_t signature_index;
    u2_t index;
  };

  std::vector<local_variable_type_table_entry>
      local_variable_type_table; /* [local_variable_type_table_length] */
};

template <>
struct attribute_info<attribute_info_type::DEPRECATED> : attribute_info_header {
};

struct annotation {
  u2_t type_index;
  u2_t num_element_value_pairs;

  std::vector<element_value_pair> /* Incomplete type here */
      element_value_pairs;        /* [num_element_value_pairs] */
};

/* TODO: Maybe move this into the type system like other descriminated unions?
 */
struct element_value {
  struct enum_const_value {
    u2_t type_name_index;
    u2_t const_name_index;
  };

  struct array_value {
    u2_t num_values;
    std::vector<element_value> values; /* [num_values] */
  };

  u1_t tag;

  union {
    u2_t const_value_index;
    enum_const_value enum_const_value;
    u2_t class_info_index;
    annotation annotation_value;
    array_value array_value;
  } value;
};

struct element_value_pair {
  u2_t element_name_index;
  element_value value;
};

template <>
struct attribute_info<attribute_info_type::RUNTIME_VISIBLE_ANNOTATIONS>
    : attribute_info_header {
  u2_t num_annotations;
  std::vector<annotation> annotations; /* [num_annotations] */
};

template <>
struct attribute_info<attribute_info_type::RUNTIME_INVISIBLE_ANNOTATIONS>
    : attribute_info_header {
  u2_t num_annotations;
  std::vector<annotation> annotations; /* [num_annotations] */
};

template <>
struct attribute_info<
    attribute_info_type::RUNTIME_VISIBLE_PARAMETER_ANNOTATIONS>
    : attribute_info_header {
  u1_t num_parameters;

  struct parameter_annotation {
    u2_t num_annotations;
    std::vector<annotation> annotations; /* [num_annotations] */
  };

  std::vector<parameter_annotation> parameter_annotations; /* num_parameters */
};

template <>
struct attribute_info<
    attribute_info_type::RUNTIME_INVISIBLE_PARAMETER_ANNOTATIONS>
    : attribute_info_header {
  u1_t num_parameters;

  struct parameter_annotation {
    u2_t num_annotations;
    std::vector<annotation> annotations; /* [num_annotations] */
  };

  std::vector<parameter_annotation> parameter_annotations; /* num_parameters */
};
} // namespace pillar

#endif
