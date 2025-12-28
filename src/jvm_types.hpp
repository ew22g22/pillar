#ifndef PILLAR_JVM_TYPES_HPP
#define PILLAR_JVM_TYPES_HPP

#include <cstdint>
#include <stdfloat>

namespace pillar {
#ifdef INT8_MAX
using byte_t = std::int8_t;
#else
#error "Exact width type std::int8_t needs to be present"
#endif

#ifdef INT16_MAX
using short_t = std::int16_t;
using char_t = short_t;
#else
#error "Exact width type std::int16_t needs to be present"
#endif

#ifdef INT32_MAX
using int_t = std::int32_t;
#else
#error "Exact width type std::int32_t needs to be present"
#endif

#ifdef INT64_MAX
using long_t = std::int64_t;
#else
#error "Exact width type std::int64_t needs to be present"
#endif

#ifdef __STDCPP_FLOAT32_T__
using float_t = std::float32_t;
#else
#error "Exact width type std::float32_t needs to be present"
#endif

#ifdef __STDCPP_FLOAT64_T__
using double_t = std::float64_t;
#else
#error "Exact width type std::float64_t needs to be present"
#endif

using boolean_t = bool;

#ifdef UINTPTR_MAX
using return_address_t = std::uintptr_t;
#else
#error "Exact width type std::uintptr_t needs to be present"
#endif
} // namespace pillar

#endif
