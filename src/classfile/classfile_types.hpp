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
} // namespace pillar

#endif
