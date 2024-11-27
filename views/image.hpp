#pragma once

#include "../util/optional.hpp"

struct image_widget : widget_base {
  
  void on(ignore, ignore) {}
  
  void paint(painter& p) {
    if (!texture)
      return;
    p.fill_style(*texture, -corner_offset, size());
    p.rectangle({0, 0}, size());
  }
  
  optional<texture_handle> texture;
  vec2i corner_offset;
};

namespace views {

struct image : view<image> {
  
  using ImgT = ::image<rgba<unsigned char>>;
  
  image(const ImgT& data, optional<vec2<int>> sz = {}) : img{data}, display_size{sz} {
    corner_offset = vec2i{0, 0};
  }
  
  auto& with_corner_offset(vec2i corner) {
    corner_offset = corner;
    return *this;
  }
  
  auto build(auto&& b, ignore) {
    auto data_ptr = img.data();
    optional<texture_handle> texture;
    if (!img.empty())
      texture = b.context().graphics_context().create_texture(img, img.shape());
    auto wsize = display_size ? *display_size : img.shape();
    return image_widget{{(vec2f) wsize}, texture, corner_offset};
  }
  
  rebuild_result rebuild(image& old, widget_ref elem, const widget_updater& up, ignore) {
    auto& w = elem.as<image_widget>();
    if (!w.texture) {
      if (!img.empty())
        w.texture = make_texture(up.context());
    }
    else {
      auto& gctx = up.context().graphics_context();
      gctx.delete_texture(*w.texture);
      
      if (img.empty()) {
        w.texture = std::nullopt;
        w.set_size({0, 0});
        return rebuild_result::size_change;
      }
      
      w.texture = gctx.create_texture(img, img.shape());
    }
    auto new_size = display_size ? *display_size : img.shape();
    return ((vec2i)w.size() != new_size) ? (w.set_size(new_size), rebuild_result::size_change)
                                  : rebuild_result{};
  }
  
  private : 
  
  texture_handle make_texture(application_context& ctx) {
    return ctx.graphics_context().create_texture(img, img.shape());
  }
  
  const ImgT& img;
  optional<vec2i> display_size;
  vec2i corner_offset;
};

} // views