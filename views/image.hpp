#pragma once

#include "../util/optional.hpp"
#include "modifiers.hpp"

namespace weave::widgets {

struct image : widget_base {
  
  optional<texture_handle> texture;
  vec2f max_size;
  vec2f corner_offset;
  
  auto size_info() const {
    widget_size_info res;
    if (!texture) {
      res.min = point{0, 0};
      res.max = point{0, 0};
      res.nominal = point{0, 0};
    }
    else {
      res.min = point{100, 100 * aspect_ratio()};
      res.max = max_size;
      res.nominal = point{150, 150};
      res.aspect_ratio = 1;
    }
    res.flex_factor = point{1, 1};
    return res;
  }
  
  point layout(point sz) const {
    if (!texture)
      return {0, 0};
    if (aspect_ratio() * sz[0] < sz[1])
      return {sz[0], sz[0] * aspect_ratio()};
    else {
      return {sz[1] / aspect_ratio(), sz[1]};
    }
  }
  
  float aspect_ratio() const {
    return max_size.y / max_size.x;
  }
  
  void on(ignore, ignore) {}
  
  void paint(painter& p) {
    if (!texture)
      return;
    p.fill_style(*texture, -corner_offset, size());
    p.fill(rectangle(size()));
    // p.stroke_style(colors::red);
    // p.stroke_rect({0, 0}, size());
  }
};

} // widgets

namespace weave::views {

struct identity {
  constexpr auto&& operator()(auto&& p) { return (p); }
};

template <class ImgT, class Proj = identity>
struct image : view<image<ImgT, Proj>>, view_modifiers {
  
  using widget_t = widgets::image;
  
  image(const observed_value<ImgT>& data, Proj proj = {}) 
  : img{data}, image_proj{proj} 
  {
  }
  
  auto& with_corner_offset(point corner) {
    corner_offset = corner;
    return *this;
  }
  
  vec2f get_display_size() const {
    if (img->empty())
      return {0, 0};
    // CAREFUL here : by convention the dimensions of images are stored as (y, x), but the size of widget is (x, y)!
    auto img_size = vec2f{(float)img->shape()[1], (float)img->shape()[0]};
    if (max_bounds) {
      if (img_size[0] == 0.f || img_size[1] == 0.f)
        return *max_bounds;
      auto aspect_ratio = img_size.y / img_size.x;
      auto result_size = vec2f{ (*max_bounds)[0], (*max_bounds)[0] * aspect_ratio };
      return result_size;
    }
    else
      return img_size;
  }
  
  /// Make an image view fit within a given box while preserving the aspect ratio.
  auto& fit(vec2f box) {
    max_bounds = box;
    return *this;
  }
  
  auto build(const build_context& ctx, ignore) {
    optional<texture_handle> texture;
    if (!img->empty())
      texture = make_texture(ctx.graphics_context());
    auto wsize = get_display_size();
    version = img.version();
    return widget_t{{wsize}, texture, wsize, corner_offset};
  }
  
  rebuild_result rebuild(const image& old, widget_ref elem, const build_context& up, ignore) {
    auto& w = elem.as<widget_t>();
    version = old.version;
    if (!w.texture) {
      if (!img->empty())
        w.texture = make_texture(up.graphics_context());
    }
    else if (version != img.version()) {
      debug_log("refreshing image");
      auto& gctx = up.graphics_context();
      gctx.delete_texture(*w.texture);
      
      if (img->empty()) {
        w.texture = std::nullopt;
        w.set_size({0, 0});
        return rebuild_result::size_change;
      }
      
      w.texture = make_texture(up.graphics_context());
    }
    auto new_size = get_display_size();
    if (w.size() == new_size)
      return {};
    w.max_size = new_size;
    w.set_size(new_size);
    return rebuild_result::size_change;
  }
  
  private : 
  
  texture_handle make_texture(graphics_context& ctx) {
    if constexpr (std::is_same_v<ImgT, weave::image<rgba<unsigned char>>>) 
      return ctx.create_texture(img.get(), img->shape());
    else {
      weave::image<rgba<unsigned char>> tex;
      tex.reshape(img->shape());
      for (int y : iota(tex.shape()[0]))
        for (int x : iota(tex.shape()[1]))
          tex(y, x) = static_cast<rgba<unsigned char>>(image_proj(img.get()(y, x)));
      return ctx.create_texture(tex, tex.shape());
    }
  }
  
  const observed_value<ImgT>& img;
  unsigned version;
  Proj image_proj;
  optional<vec2f> max_bounds;
  point corner_offset = {0, 0};
};

} // views