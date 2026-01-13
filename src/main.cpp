#include <array>
#include <print>

#include "classfile/classfile.hpp"

auto main(int argc, char **argv) -> int {
  static auto constexpr PROJECT_NAME = "Pillar";

  static auto constexpr test_buf = std::to_array({
      static_cast<std::byte>(0xCA),
      static_cast<std::byte>(0xFE),
      static_cast<std::byte>(0xBA),
      static_cast<std::byte>(0xBE),
      /* Minor version of 00 */
      static_cast<std::byte>(0x00),
      static_cast<std::byte>(0x00),
      /* Major version of 69 */
      static_cast<std::byte>(0x00),
      static_cast<std::byte>(0x45),
      /* CP count of 2 */
      static_cast<std::byte>(0x00),
      static_cast<std::byte>(0x02),
      /* CP index one:
      TAG:   0x03 (Integer)
      VALUE: 1 (0x00 0x00 0x00 0x01) */
      static_cast<std::byte>(0x03),
      static_cast<std::byte>(0x00),
      static_cast<std::byte>(0x00),
      static_cast<std::byte>(0x00),
      static_cast<std::byte>(0x01),
      /* CP index one:
      TAG:   0x04 (Float)
      VALUE: 2 (0x00 0x00 0x00 0x01) */
      static_cast<std::byte>(0x04),
      static_cast<std::byte>(0x00),
      static_cast<std::byte>(0x00),
      static_cast<std::byte>(0x00),
      static_cast<std::byte>(0x02),
  });

  auto constexpr test_span = std::span<const std::byte>{test_buf};

  auto cf = pillar::classfile::parse_from_bytes(test_span);
  auto cp1 = std::get<pillar::cp_info<pillar::cp_info_tag::INTEGER>>(
      cf.value().constant_pool[0]);
  auto cp2 = std::get<pillar::cp_info<pillar::cp_info_tag::FLOAT>>(
      cf.value().constant_pool[1]);

  std::println("Hello from {}: {} {}", PROJECT_NAME, cp1.bytes, cp2.bytes);
  return 0;
}
