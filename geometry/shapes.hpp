#pragma once

#include <util/vec.hpp>

#include <numbers>

namespace weave::geo {

template <class T>
struct rectangle {
  
  constexpr rectangle(vec2<T> size) : origin(0, 0), size{size} {}
  constexpr rectangle(vec2<T> origin, vec2<T> size) : origin{origin}, size{size} {}
  
  constexpr bool contains(vec2<T> p) const {
    return p.x >= origin.x && p.x <= origin.x + size.x 
           && p.y >= origin.y && p.y <= origin.y + size.y;  
  }
  
  constexpr rectangle translated(vec2<T> p) const {
    auto Res{*this};
    Res.origin += p;
    return Res;
  }
  
  vec2<T> origin, size;
};

template <class T>
struct circle {

  constexpr bool contains(vec2<T> pt) const {
    auto dist = center - pt;
    dist *= dist;
    return dist.x + dist.y < radius * radius;
  }
  
  /* 
  template <class V>
  constexpr auto point(radians<V> r) const {
    return center + vec2<T>{radius * std::cos(r.value), radius * std::sin(r.value)};
  }  
  
  template <class V>
  constexpr circle<V> to() const {
    return { center.template to<V>(), static_cast<V>(radius) };
  } */ 
  
  constexpr T area() const {
    return std::numbers::pi * radius * radius;
  }
  
  constexpr circle translated(vec2<T> p) const {
    auto Res{*this};
    Res.center += p;
    return Res;
  }
  
  vec2<T> center;
  T radius;
};

template <class T>
circle(vec2<T>, T) -> circle<T>;

template <class T>
struct triangle {
	
	/* 
	constexpr triangle(const circle<T>& c, radians<T> r)
	: a{ c.point(r) },
	  b{ c.point(r + radians<T>{2 * std::numbers::pi / 3}) },
	  c{ c.point(r + radians<T>{2 * 2 * std::numbers::pi / 3}) }
	{
	} */ 
	
  constexpr triangle(const vec2<T>& a_, const vec2<T>& b_, const vec2<T>& c_)
  : a{ a_ },
    b{ b_ },
    c{ c_ }
  {
  }
  
  constexpr auto two_area() const {
    return ( -b.y * c.x + a.y * (-b.x + c.x) + a.x * (b.y - c.y) + b.x * c.y );
  }
  
  constexpr auto area() const { 
    return two_area() / 2; 
  }
  
  constexpr auto centroid() const { 
    return (a + b + c) / 3; 
  }
  
  constexpr bool contains(const vec2<T>& p) const
  {
    const auto two_area_ = two_area();
    
    // barycentric coordinates
    const auto s = (a.y * c.x - a.x * c.y + (c.y - a.y) * p.x + (a.x - c.x) * p.y) / two_area_;
    
    if (s < 0)
      return false;
    
    const auto t = (a.x * b.y - a.y * b.x + (a.y - b.y) * p.x + (b.x - a.x) * p.y) / two_area_;
    
    return (t >= 0) && (s + t <= 1);
  }
  
  constexpr auto translated(const vec2<T>& p) const {
    auto Res{*this};
    Res.a += p;
    Res.b += p;
    Res.c += p;
    return Res;
  }
  
  /* 
  template <class V>
  constexpr triangle<V> to() const {
    return triangle<V>{ a.template to<V>(), b.template to<V>(), c.template to<V>() };
  } */ 
  
  vec2<T> a, b, c;
};

} // weave::geo