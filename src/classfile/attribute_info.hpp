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

  std::vector<struct element_value_pair> /* Incomplete type here */
      element_value_pairs;               /* [num_element_value_pairs] */
};

enum struct element_value_tag : u1_t {
  BYTE = 'B',
  CHAR = 'C',
  DOUBLE = 'D',
  FLOAT = 'F',
  INT = 'I',
  LONG = 'J',
  SHORT = 'S',
  BOOLEAN = 'Z',
  STRING = 's',
  ENUM = 'e',
  CLASS = 'c',
  ANNOTATION = '@',
  ARRAY = '['
};

template <element_value_tag Tag>
struct element_value;

struct element_value<element_value_tag::BYTE> {
  u2_t const_value_index;
};

struct element_value<element_value_tag::CHAR> {
  u2_t const_value_index;
};

struct element_value<element_value_tag::DOUBLE> {
  u2_t const_value_index;
};

struct element_value<element_value_tag::FLOAT> {
  u2_t const_value_index;
};

struct element_value<element_value_tag::INT> {
  u2_t const_value_index;
};

struct element_value<element_value_tag::LONG> {
  u2_t const_value_index;
};

struct element_value<element_value_tag::SHORT> {
  u2_t const_value_index;
};

struct element_value<element_value_tag::BOOLEAN> {
  u2_t const_value_index;
};

struct element_value<element_value_tag::STRING> {
  u2_t const_value_index;
};

struct element_value<element_value_tag::ENUM> {
  u2_t type_and_name_index;
  u2_t const_name_index;
};

struct element_value<element_value_tag::CLASS> {
  u2_t class_info_index;
};

struct element_value<element_value_tag::ANNOTATION> {
  annotation annotation_value;
};

struct element_value<element_value_tag::ARRAY> {
  u2_t num_values;
  std::vector<element_value_t> values; /* [num_values] */
};

template <element_value_tag... Tags>
using element_values_t = std::variant<element_value<Tags>...>;
using element_value_t
    = element_values_t<element_value_tag::BYTE, element_value_tag::CHAR,
                       element_value_tag::DOUBLE, element_value_tag::FLOAT,
                       element_value_tag::INT, element_value_tag::LONG,
                       element_value_tag::SHORT, element_value_tag::BOOLEAN,
                       element_value_tag::STRING, element_value_tag::ENUM,
                       element_value_tag::CLASS, element_value_tag::ANNOTATION,
                       element_value_tag::ARRAY>;

struct element_value_pair {
  u2_t element_name_index;
  element_value_t value;
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

struct parameter_annotation {
  u2_t num_annotations;
  std::vector<annotation> annotations; /* [num_annotations] */
};

template <>
struct attribute_info<
    attribute_info_type::RUNTIME_VISIBLE_PARAMETER_ANNOTATIONS>
    : attribute_info_header {
  u1_t num_parameters;
  std::vector<parameter_annotation> parameter_annotations; /* num_parameters */
};

template <>
struct attribute_info<
    attribute_info_type::RUNTIME_INVISIBLE_PARAMETER_ANNOTATIONS>
    : attribute_info_header {
  u1_t num_parameters;
  std::vector<parameter_annotation> parameter_annotations; /* num_parameters */
};

struct type_path {
  u1_t path_length;

  struct path_entry {
    u1_t type_path_kind;
    u1_t type_argument_index;
  };

  std::vector<path_entry> path; /* [path_length] */
};

enum struct type_annotation_tag : u1_t {
  GENERIC_CLASS = 0x00,
  GENERIC_METHOD = 0x01,
  EXTENDS = 0x10,
  GENERIC_CLASS_BOUND = 0x11,
  GENERIC_METHOD_BOUND = 0x12,
  FIELD = 0x13,
  RETURN = 0x14,
  RECEIVER = 0x15,
  PARAMETER = 0x16,
  THROWS = 0x17,
  LOCAL = 0x40,
  RESOURCE = 0x41,
  EXCEPTION = 0x42,
  INSTANCEOF = 0x43,
  NEW = 0x44,
  METHOD_NEW = 0x45,
  METHOD_IDENTIFIER = 0x46,
  CAST = 0x47,
  GENERIC_NEW = 0x48,
  GENERIC_METHOD_INVOCATION = 0x49,
  GENERIC_METHOD_NEW = 0x4A,
  GENERIC_METHOD_IDENTIFIER = 0x4B,
};

template <type_annotation_tag Tag>
struct type_annotation_target;

struct type_annotation_target<type_annotation_tag::GENERIC_CLASS> {
  u1_t type_parameter_target;
};

struct type_annotation_target<type_annotation_tag::GENERIC_METHOD> {
  u1_t type_parameter_target;
};

struct type_annotation_target<type_annotation_tag::EXTENDS> {
  u2_t supertype_target;
};

struct type_annotation_target<type_annotation_tag::GENERIC_CLASS_BOUND> {
  u1_t type_parameter_index;
  u1_t bound_index;
};

struct type_annotation_target<type_annotation_tag::GENERIC_METHOD_BOUND> {
  u1_t type_parameter_index;
  u1_t bound_index;
};

struct type_annotation_target<type_annotation_tag::FIELD> {};

struct type_annotation_target<type_annotation_tag::RETURN> {};

struct type_annotation_target<type_annotation_tag::RECEIVER> {};

struct type_annotation_target<type_annotation_tag::PARAMETER> {
  u1_t formal_parameter_index;
};

struct type_annotation_target<type_annotation_tag::THROWS> {
  u2_t throws_type_index;
};

struct localvar_table_entry {
  u2_t start_pc;
  u2_t length;
  u2_t index;
};

struct type_annotation_target<type_annotation_tag::LOCAL> {
  u2_t table_length;
  std::vector<localvar_table_entry> table;
};

struct type_annotation_target<type_annotation_tag::RESOURCE> {
  u2_t table_length;
  std::vector<localvar_table_entry> table;
};

struct type_annotation_target<type_annotation_tag::EXCEPTION> {
  u2_t exception_table_index;
};

struct type_annotation_target<type_annotation_tag::INSTANCEOF> {
  u2_t offset;
};

struct type_annotation_target<type_annotation_tag::NEW> {
  u2_t offset;
};

struct type_annotation_target<type_annotation_tag::METHOD_NEW> {
  u2_t offset;
};

struct type_annotation_target<type_annotation_tag::METHOD_IDENTIFIER> {
  u2_t offset;
};

struct type_annotation_target<type_annotation_tag::CAST> {
  u2_t offset;
  u1_t type_argument_index;
};

struct type_annotation_target<type_annotation_tag::GENERIC_NEW> {
  u2_t offset;
  u1_t type_argument_index;
};

struct type_annotation_target<type_annotation_tag::GENERIC_METHOD_INVOCATION> {
  u2_t offset;
  u1_t type_argument_index;
};

struct type_annotation_target<type_annotation_tag::GENERIC_METHOD_NEW> {
  u2_t offset;
  u1_t type_argument_index;
};

struct type_annotation_target<type_annotation_tag::GENERIC_METHOD_IDENTIFIER> {
  u2_t offset;
  u1_t type_argument_index;
};

template <type_annotation_tag... Tags>
using type_annotation_targets_t = std::variant<type_annotation_target<Tags>...>;

using type_annotation_target_t = type_annotation_targets_t<
    type_annotation_tag::GENERIC_CLASS, type_annotation_tag::GENERIC_METHOD,
    type_annotation_tag::EXTENDS, type_annotation_tag::GENERIC_CLASS_BOUND,
    type_annotation_tag::GENERIC_METHOD_BOUND, type_annotation_tag::FIELD,
    type_annotation_tag::RETURN, type_annotation_tag::RECEIVER,
    type_annotation_tag::PARAMETER, type_annotation_tag::THROWS,
    type_annotation_tag::LOCAL, type_annotation_tag::RESOURCE,
    type_annotation_tag::EXCEPTION, type_annotation_tag::INSTANCEOF,
    type_annotation_tag::NEW, type_annotation_tag::METHOD_NEW,
    type_annotation_tag::METHOD_IDENTIFIER, type_annotation_tag::CAST,
    type_annotation_tag::GENERIC_NEW,
    type_annotation_tag::GENERIC_METHOD_INVOCATION,
    type_annotation_tag::GENERIC_METHOD_NEW,
    type_annotation_tag::GENERIC_METHOD_IDENTIFIER>;

struct type_annotation {
  type_annotation_target_t target_info;
  u2_t type_index;
  u2_t num_element_value_pairs;
  std::vector<element_value_pair>
      element_value_pairs; /* [num_element_value_pairs] */
};

template <>
struct attribute_info<attribute_info_type::RUNTIME_VISIBLE_TYPE_ANNOTATIONS>
    : attribute_info_header {
  u2_t num_annotations;
  std::vector<type_annotation> annotations; /* [num_annotations] */
};

template <>
struct attribute_info<attribute_info_type::RUNTIME_INVISIBLE_TYPE_ANNOTATIONS>
    : attribute_info_header {
  u2_t num_annotations;
  std::vector<type_annotation> annotations; /* [num_annotations] */
};

template <>
struct attribute_info<attribute_info_type::ANNOTATION_DEFAULT>
    : attribute_info_header {
  element_value_t element_value;
};

template <>
struct attribute_info<attribute_info_type::BOOTSTRAP_METHODS>
    : attribute_info_header {
  u2_t num_bootstrap_methods;

  struct bootstrap_method_entry {
    u2_t bootstrap_method_ref;
    u2_t num_bootstrap_arguments;
    std::vector<u2_t> bootstrap_arguments; /* [num_bootstrap_arguments] */
  };

  std::vector<bootstrap_method_entry>
      bootstrap_methods; /* [num_bootstrap_methods] */
};

template <>
struct attribute_info<attribute_info_type::METHOD_PARAMETERS>
    : attribute_info_header {
  u1_t parameters_count;

  struct method_parameter_entry {
    u2_t name_index;
    u2_t access_flags;
  };

  std::vector<method_parameter_entry> parameters; /* [parameters_count] */
};
} // namespace pillar

#endif
