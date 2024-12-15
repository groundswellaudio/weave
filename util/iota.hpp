#pragma once

#include <ranges>

inline constexpr auto iota(int k) {
  return std::ranges::iota_view(0, k);
}

inline constexpr auto iota(int a, int b) {
  return std::ranges::iota_view(a, b);
}

template <class Range>
inline constexpr auto enumerate(Range&& range) {
  return std::views::join((Range&&) range, iota(range.size()));
}