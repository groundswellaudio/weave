#pragma once

#include <limits>

template <class T>
struct rgba;

// RGB color in gamma 2.2
template <class T>
struct rgb {
  
  static constexpr auto norm() {
    if constexpr (^T == ^float or ^T == ^double)
      return 1.f;
    else
      return std::numeric_limits<T>::max();
  }
  
  constexpr T& operator[](int x)              { return data[x]; }
  constexpr const T& operator[](int x) const  { return data[x]; }
  
  template <class V>
  constexpr operator rgb<V> () const 
  {
    static constexpr double norm_factor = (double) rgb<V>::norm() / norm();
    return { (V)(data[0] * norm_factor), (V)(data[1] * norm_factor), (V)(data[2] * norm_factor)}; 
  }
  
  template <class V>
  constexpr operator rgba<V> () const;

  T data[3];
};

rgb(float, float, float) -> rgb<float>;

// RGBA color in gamma 2.2 space
template <class T>
struct rgba {

  constexpr T& operator[](int x)              { return x == 4 ? alpha : col[x]; }
  constexpr const T& operator[](int x) const  { return x == 4 ? alpha : col[x]; }

  constexpr auto with_alpha(T alpha_) const {
    return rgba{col, alpha_};
  }

  rgb<T> col;
  T alpha;
};

template <class T>
template <class V>
constexpr rgb<T>::operator rgba<V>() const {
  return {*this, 1.f};
}

using rgba_f = rgba<float>;
using rgb_u8 = rgb<unsigned char>;
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