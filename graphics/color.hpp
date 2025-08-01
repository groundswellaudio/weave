#pragma once

#include <limits>
#include <cmath>
#include <type_traits>

namespace weave {

enum class rgb_encoding {
  linear, 
  gamma22
};

template <class T>
struct rgba;

/* 
consteval void color_operators(class_builder& b) {
  b << ^^ struct C {
    template <operator_kind Op>
      requires (is_compound_assign(Op))
    constexpr auto& apply_op(const C& o) {
      for (int k = 0; k < channels; ++k)
        ([:make_operator_expr(Op, ^^((*this)[k]), ^^(o[k])):]);
      return *this;
    }
    
    template <class O, operator_kind Op>
      requires (!is_compound_assign(Op))
    constexpr auto apply_op(const O& o) const {
      auto res {*this};
      ([:make_operator_expr(compound_equivalent(Op), ^^(res), ^^(o)):]);
      return res;
    }
    
    template<operator_kind Op>
      requires (is_compound_assign(Op))
    constexpr auto& apply_op(C::scalar o) {
      for (int k = 0; k < channels; ++k)
        ([:make_operator_expr(Op, ^^((*this)[k]), ^^(o)):]);
      return *this;
    }
    
    [:declare_arithmetic(^^const C&):];
    [:declare_arithmetic(^^T):];
  };
} */ 

// RGB color in gamma 2.2
template <class T>
struct rgb {
  
  using scalar = T;
  static constexpr auto channels = 3;
  
  static constexpr auto norm() {
    if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>)
      return 1.f;
    else
      return std::numeric_limits<T>::max();
  }
  
  constexpr T& operator[](int x)              { return data[x]; }
  constexpr const T& operator[](int x) const  { return data[x]; }
  
  template <class V>
  constexpr operator rgb<V> () const 
  {
    static constexpr double norm_factor = ((double) rgb<V>::norm()) / norm();
    return { (V)(data[0] * norm_factor), (V)(data[1] * norm_factor), (V)(data[2] * norm_factor)}; 
  }
  
  constexpr bool operator==(const rgb<T>& o) const = default;
  
  template <class V>
  constexpr operator rgba<V> () const;
  
  constexpr rgb operator*(T x) const {
    return {data[0] * x, data[1] * x, data[2] * x};
  }
  
  constexpr rgb operator/(T x) const {
    return {data[0] / x, data[1] / x, data[2] / x};
  }
  
  // [:color_operators()];
  
  T data[3];
};

template <class T>
using distance_return_type = std::conditional_t<std::is_integral_v<T>, int, T>; 

template <class T, class X = distance_return_type<T>>
constexpr X distance_squared(const rgb<T>& a, const rgb<T>& b) 
{
  X res = 0;
  for (int k = 0; k < 3; ++k) {
    auto diff = a[k] - b[k];
    res += diff * diff;
  }
  return res;
}

template <class T>
constexpr auto distance(const rgb<T>& a, const rgb<T>& b)
{ 
  using std::sqrt;
  return sqrt(distance_squared(a, b));
}

rgb(float, float, float) -> rgb<float>;

template <class T>
constexpr rgb<T> sqrt(const rgb<T>& r) {
  using std::sqrt;
  return {sqrt(r[0]), sqrt(r[1]), sqrt(r[2])};
}

// RGBA color in gamma 2.2 space
template <class T>
struct rgba {
  
  using scalar = T;
  static constexpr auto channels = 4;
  
  static constexpr auto norm() { return rgb<T>::norm(); }
  
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
  
  template <class V>
  constexpr operator rgb<V> () const {
    return static_cast<rgba<V>>(*this).col;
  }
  
  constexpr bool operator==(const rgba<T>& o) const = default;

  constexpr rgba operator*(T x) const {
    return {col * x, alpha * x};
  }
  
  constexpr rgba operator/(T x) const {
    return {col / x, alpha / x};
  }
  
  // [:color_operators()];
  
  rgb<T> col;
  T alpha = norm();
};


template <class T>
template <class V>
constexpr rgb<T>::operator rgba<V>() const {
  return {*this, rgb<V>::norm()};
}

template <class T, class X = distance_return_type<T>>
constexpr X distance_squared(const rgba<T>& a, const rgba<T>& b) 
{
  X res = 0;
  for (int k = 0; k < 4; ++k) {
    auto diff = a[k] - b[k];
    res += diff * diff;
  }
  return res;
}

template <class T>
constexpr auto distance(const rgba<T>& a, const rgba<T>& b) 
{
  using std::sqrt;
  return sqrt(distance_squared(a, b));
}

template <class T>
struct monochrome {
  constexpr auto& operator=(T v) { value = v; return *this; }
  constexpr bool operator==(const monochrome& o) const = default; 
  T value;
};

using rgba_f = rgba<float>;
using rgb_f = rgb<float>;
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

} // weave