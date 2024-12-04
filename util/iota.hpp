#pragma once

#include <ranges>

inline auto iota(int k) {
  return std::ranges::iota_view(0, k);
}