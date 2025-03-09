#pragma once

#include <initializer_list>
#include <vector>
#include <span>
#include <string>

#include "util/vec.hpp"
#include "util/optional.hpp"

#include "stb_image.h"
#include "stb_image_write.h"

namespace weave {

template <class Pixel>
struct image; 

/// Image constructor from raw pointer.
template <class Pixel, class T>
image<Pixel> make_image_from_raw(const T* ptr, vec2i sz); 

/// Decode a compressed image buffer
inline optional<image<rgba<unsigned char>>> 
  decode_image(std::span<const unsigned char> data);

inline optional<image<rgba<unsigned char>>> load_image_from_file(const std::string& path); 

inline bool save_image_to_file(const std::string& path, 
                               const image<rgba<unsigned char>>& img);


template <class Pixel>
struct image {
  
  image(const image& o) = default;
  image() = default;
  image(vec2i size) {
    reshape(size);
  }
  
  template <class V>
  image(vec2i size, std::initializer_list<V> inits) 
  {
    for (auto i : inits)
      buffer.emplace_back(i);
    reshape(size);
  }
  
  using pixel_type = Pixel;
  
  auto&& operator()(this auto&& self, int y, int x) {
    assert( y >= 0 && y < self.shape()[0] && x >= 0 && x < self.shape()[1] );
    return self.buffer[y * self.shape()[1] + x];
  }
  
  auto&& operator() (this auto&& self, vec2i pt) {
    return self(pt[0], pt[1]);
  }
  
  template <class P2>
  image<P2> to(this const auto& self) {
    image<P2> res;
    res.reshape(self.shape());
    for (int y = 0; y < self.shape()[0]; ++y)
      for (int x = 0; x < self.shape()[1]; ++x)
        res(y, x) = static_cast<P2>(self(y, x));
    return res;
  }
  
  void reshape(vec2i new_shape) {
    buffer.resize(new_shape.x * new_shape.y);
    shape_v = new_shape;
  }
  
  bool empty() const { return buffer.empty(); }
  auto data() const { return buffer.data(); }
  auto shape() const { return shape_v; }
  
  auto begin(this auto& self) { return self.buffer.begin(); }
  auto end(this auto& self) { return self.buffer.end(); }
  
  bool operator==(const image& o) const {
    if (shape() != o.shape())
      return false;
    for (int x0 : iota(shape()[0]))
      for (int x1 : iota(shape()[1]))
        if ((*this)({x0, x1}) != o({x0, x1}))
          return false;
    return true;
  }
  
  auto size() const { return buffer.size(); }
  
  std::vector<Pixel> buffer;
  vec2i shape_v {0, 0};
};

template <class Pixel, class T>
image<Pixel> make_image_from_raw(const T* ptr, vec2<int> sz) {
  image<Pixel> res;
  res.buffer.resize(sz.x * sz.y);
  res.shape_v = sz;
  std::memcpy(res.buffer.data(), ptr, res.buffer.size() * sizeof(Pixel));
  return res;
}

} // weave