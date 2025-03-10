#pragma once

#include <numbers>

namespace weave {

template <class S, class T>
struct angle_base {
  
  constexpr auto& operator+=(this T& self, T o) {
    self.value += o.value; 
    return self;
  }
  
  constexpr auto& operator-=(this T& self, T o) {
    self.value -= o.value;
    return self;
  }
  
  constexpr auto& operator*=(this T& self, T o) {
    self.value *= o.value; 
    return self;
  }
  
  constexpr auto& operator/=(this T& self, T o) {
    self.value /= o.value; 
    return self;
  }
  
  constexpr auto operator+(this T self, T o) {
    auto Res{self};
    Res += o;
    return Res;
  }
  
  constexpr auto operator-(this T self, T o) {
    auto Res{self};
    Res -= o;
    return Res;
  }
  
  constexpr auto operator*(this T self, T o) {
    auto Res{self};
    Res *= o;
    return Res;
  }
  
  constexpr auto operator/(this T self, T o) {
    auto Res{self};
    Res /= o;
    return Res;
  }
  
  S value; 
};

template <class T>
struct degrees; 

template <class T>
struct radians : angle_base<T, radians<T>> {
  
  constexpr radians(T val) : angle_base<T, radians<T>>{val} {}
  
  constexpr operator degrees<T> () const;
};

template <class T>
radians(T) -> radians<T>; 

template <class T>
struct degrees : angle_base<T, degrees<T>> {
  
  constexpr degrees(T val) : angle_base<T, degrees<T>>{val} {}
  
  constexpr operator radians<T> () const {
    return { (T) (this->value * 2 * std::numbers::pi / 360.f) };
  }
  
};

template <class T>
degrees(T) -> degrees<T>;

template <class T>
constexpr radians<T>::operator degrees<T> () const {
  return { (T) (this->value * 360.f / (2 * std::numbers::pi)) };
}

} // weave