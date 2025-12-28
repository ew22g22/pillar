#include <print>

auto main(int argc, char **argv) -> int {
  static auto constexpr PROJECT_NAME = "Pillar";

  std::println("Hello from {}.", PROJECT_NAME);
  return 0;
}
