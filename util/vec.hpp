#pragma once

#include <type_traits>

namespace weave {

template <class T, unsigned N>
struct vec 
{
  T data[N];
  
  constexpr bool operator==(const vec& o) const = default; 
  
  decltype(auto) operator[](this auto&& self, int index) {
    return (self.data[index]);
  }
  
  // Mutating operators 
  
  constexpr auto& operator+=(vec o) {
    for (int k = 0; k < N; ++k)
      data += o.data[k];
    return *this;
  }
  
  constexpr auto& operator-=(vec o) {
    for (int k = 0; k < N; ++k)
      data -= o.data[k];
    return *this;
  }
  
  constexpr auto& operator+=(T o) {
   for (int k = 0; k < N; ++k)
      data += o;
    return *this;
  }
  
  constexpr auto& operator-=(T o) {
    for (int k = 0; k < N; ++k)
      data -= o;
    return *this;
  }
  
  constexpr auto& operator*=(T o) {
    for (int k = 0; k < N; ++k)
      data *= o;
    return *this;
  }
  
  constexpr auto& operator/=(T o) {
    for (int k = 0; k < N; ++k)
      data /= o;
    return *this;
  }
  
  // Pure operators
  
  constexpr auto operator+(vec o) const {
    auto Res{*this};
    Res += o;
    return Res;
  }
  
  constexpr auto operator-(vec o) const {
    auto Res{*this};
    Res -= o;
    return Res;
  }
  
  constexpr auto operator+(T o) const {
    auto Res{*this};
    Res += o;
    return Res;
  }
  
  constexpr auto operator-(T o) const {
    auto Res {*this};
    Res -= o;
    return Res;
  }
  
  constexpr auto operator*(T o) const {
    auto Res{*this};
    Res *= o;
    return Res;
  }
  
  constexpr auto operator/(T o) const {
    auto Res {*this};
    Res /= o;
    return Res;
  }
};

template <class T>
struct vec<T, 2> {
  
  T x, y;
  
  decltype(auto) operator[](this auto&& self, int index) {
    return (index ? self.y : self.x);
  }
  
  constexpr bool operator==(const vec& o) const = default; 
  
  // Mutating operators
  
  constexpr auto& operator+=(vec o) {
    x += o.x;
    y += o.y;
    return *this;
  }
  
  constexpr auto& operator-=(vec o) {
    x -= o.x;
    y -= o.y;
    return *this;
  }
  
  constexpr auto& operator+=(T o) {
    x += o;
    y += o;
    return *this;
  }
  
  constexpr auto& operator-=(T o) {
    x -= o;
    y -= o;
    return *this;
  }
  
  constexpr auto& operator*=(T o) {
    x *= o;
    y *= o;
    return *this;
  }
  
  constexpr auto& operator/=(T o) {
    x /= o;
    y /= o;
    return *this;
  }
  
  // Pure operators
  
  constexpr auto operator+(vec o) const {
    auto Res{*this};
    Res += o;
    return Res;
  }
  
  constexpr auto operator-(vec o) const {
    auto Res{*this};
    Res -= o;
    return Res;
  }
  
  constexpr auto operator+(T o) const {
    auto Res{*this};
    Res += o;
    return Res;
  }
  
  constexpr auto operator-(T o) const {
    auto Res {*this};
    Res -= o;
    return Res;
  }
  
  constexpr auto operator*(T o) const {
    auto Res{*this};
    Res *= o;
    return Res;
  }
  
  constexpr auto operator/(T o) const {
    auto Res {*this};
    Res /= o;
    return Res;
  }
};

template <class U, class T, unsigned N>
  requires (std::is_scalar_v<U>)
constexpr vec<T, N> operator*(U x, vec<T, N> v) {
  return v * x;
}

template <class T>
constexpr vec<T, 2> operator-(vec<T, 2> v) {
  return {-v.x, -v.y};
}

template <class T, unsigned N>
constexpr vec<T, N> operator-(const vec<T, N>& v) {
  auto Res = v;
  for (auto& e : Res)
    e = -e;
  return Res;
}

template <class T>
constexpr vec<T, 2> min(vec<T, 2> a, vec<T, 2> b) {
  return {std::min(a.x, b.x), std::min(a.y, b.y)};
}

template <class T>
constexpr vec<T, 2> max(vec<T, 2> a, vec<T, 2> b) {
  return {std::max(a.x, b.x), std::max(a.y, b.y)};
}

template <class T>
using vec2 = vec<T, 2>;

using vec2f = vec2<float>;
using vec2d = vec2<double>;
using vec2i = vec2<int>;
using vec2u = vec2<unsigned>;
using vec2b = vec2<bool>;

} // weave
