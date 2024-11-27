#pragma once

#include <limits>
#include <cmath>

enum class rgb_encoding {
  linear, 
  gamma22
};

template <class T, rgb_encoding>
struct rgba;

// RGB color in gamma 2.2
template <class T, rgb_encoding E = rgb_encoding::gamma22>
struct rgb {
  
  using scalar = T;
  static constexpr auto channels = 3;
  
  static constexpr auto norm() {
    if constexpr (^T == ^float or ^T == ^double)
      return 1.f;
    else
      return std::numeric_limits<T>::max();
  }
  
  constexpr T& operator[](int x)              { return data[x]; }
  constexpr const T& operator[](int x) const  { return data[x]; }
  
  template <class V>
  constexpr operator rgb<V, E> () const 
  {
    static constexpr double norm_factor = ((double) rgb<V>::norm()) / norm();
    return { (V)(data[0] * norm_factor), (V)(data[1] * norm_factor), (V)(data[2] * norm_factor)}; 
  }
  
  constexpr bool operator==(const rgb<T>& o) const = default;
  
  template <class V>
  constexpr operator rgba<V, E> () const;

  T data[3];
};

template <class T>
using distance_return_type = %( is_integral(^T) ? ^int : ^T );

template <class T, class X = distance_return_type<T>, rgb_encoding E>
constexpr X distance_squared(const rgb<T, E>& a, const rgb<T, E>& b) 
{
  X res;
  for (int k = 0; k < 3; ++k) {
    auto diff = a[k] - b[k];
    res += diff * diff;
  }
  return res;
}

template <class T, rgb_encoding E>
constexpr auto distance(const rgb<T, E>& a, const rgb<T, E>& b)
{ 
  using std::sqrt;
  return sqrt(distance_squared(a, b));
}

rgb(float, float, float) -> rgb<float>;

// RGBA color in gamma 2.2 space
template <class T, rgb_encoding E = rgb_encoding::gamma22>
struct rgba {
  
  using scalar = T;
  static constexpr auto channels = 4;
  
  static constexpr auto norm() { return rgb<T, E>::norm(); }
  
  constexpr T& operator[](int x)              { return x == 4 ? alpha : col[x]; }
  constexpr const T& operator[](int x) const  { return x == 4 ? alpha : col[x]; }
  
  constexpr auto with_alpha(T alpha_) const {
    return rgba{col, alpha_};
  }
  
  template <class V>
  constexpr operator rgba<V> () const {
    static constexpr double norm_factor = ((double) rgb<V>::norm()) / rgb<T>::norm();
    return { 
      {(V)(col[0] * norm_factor), (V)(col[1] * norm_factor), (V)(col[2] * norm_factor)},
      (V)(alpha * norm_factor)
    };
  }
  
  constexpr bool operator==(const rgba<T>& o) const = default;

  rgb<T> col;
  T alpha = norm();
};


template <class T, rgb_encoding E>
template <class V>
constexpr rgb<T, E>::operator rgba<V, E>() const {
  return {*this, rgb<V>::norm()};
}

template <class T, class X = distance_return_type<T>, rgb_encoding E>
constexpr X distance_squared(const rgba<T, E>& a, const rgba<T, E>& b) 
{
  X res;
  for (int k = 0; k < 4; ++k) {
    auto diff = a[k] - b[k];
    res += diff * diff;
  }
  return res;
}

template <class T, rgb_encoding E>
constexpr auto distance(const rgba<T, E>& a, const rgba<T, E>& b) 
{
  using std::sqrt;
  return sqrt(distance_squared(a, b));
}

using rgba_f = rgba<float>;
using rgb_u8 = rgb<unsigned char>;
using rgba_u8 = rgba<unsigned char>;
using color  = rgba<float>;

namespace colors 
{ 
  static constexpr rgb_u8 black   {0, 0, 0};
  static constexpr rgb_u8 white   {255, 255, 255};
  static constexpr rgb_u8 red     {255, 0, 0};
  static constexpr rgb_u8 lime    {0, 255, 0};
  static constexpr rgb_u8 blue    {0, 0, 255};
  static constexpr rgb_u8 yellow  {255, 255, 0};
  static constexpr rgb_u8 cyan    {0, 255, 255};
  static constexpr rgb_u8 magenta {255, 0, 255};
  static constexpr rgb_u8 silver  {192, 192, 192};
  static constexpr rgb_u8 gray    {128, 128, 128};
  static constexpr rgb_u8 green   {0, 128, 0};
  static constexpr rgb_u8 maroon  {128, 0, 0};
  static constexpr rgb_u8 purple  {128, 0, 128};
}