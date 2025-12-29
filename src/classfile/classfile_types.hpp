#ifndef PILLAR_CLASSFILE_CLASSFILE_TYPES_HPP
#define PILLAR_CLASSFILE_CLASSFILE_TYPES_HPP

#include <cstdint>

namespace pillar {
#ifdef UINT32_MAX
using u1_t = std::uint8_t;
using u2_t = std::uint16_t;
using u4_t = std::uint32_t;
#else
#error "Exact width types need to be present"
#endif

inline auto constexpr ACC_PUBLIC = static_cast<u2_t>(0x0001);
inline auto constexpr ACC_PRIVATE = static_cast<u2_t>(0x0002);
inline auto constexpr ACC_PROTECTED = static_cast<u2_t>(0x0004);
inline auto constexpr ACC_STATIC = static_cast<u2_t>(0x0008);
inline auto constexpr ACC_FINAL = static_cast<u2_t>(0x0010);
inline auto constexpr ACC_SUPER = static_cast<u2_t>(0x0020);
inline auto constexpr ACC_SYNCHRONIZED = static_cast<u2_t>(0x0020);
inline auto constexpr ACC_VOLATILE = static_cast<u2_t>(0x0040);
inline auto constexpr ACC_BRIDGE = static_cast<u2_t>(0x0020);
inline auto constexpr ACC_TRANSIENT = static_cast<u2_t>(0x0080);
inline auto constexpr ACC_VARARGS = static_cast<u2_t>(0x0020);
inline auto constexpr ACC_NATIVE = static_cast<u2_t>(0x0100);
inline auto constexpr ACC_INTERFACE = static_cast<u2_t>(0x0200);
inline auto constexpr ACC_ABSTRACT = static_cast<u2_t>(0x0400);
inline auto constexpr ACC_STRICT = static_cast<u2_t>(0x0800);
inline auto constexpr ACC_SYNTHETIC = static_cast<u2_t>(0x1000);
inline auto constexpr ACC_ANNOTATION = static_cast<u2_t>(0X2000);
inline auto constexpr ACC_ENUM = static_cast<u2_t>(0X4000);
inline auto constexpr ACC_MODULE = static_cast<u2_t>(0x8000);

} // namespace pillar

#endif
