#pragma once

#include "../util/optional.hpp"

struct image_widget : widget_base {
  
  void on(ignore, ignore) {}
  
  void paint(painter& p) {
    if (!texture)
      return;
    p.fill_style(*texture, {0, 0}, size());
    p.rectangle({0, 0}, size());
  }
  
  optional<texture_handle> texture;
};

struct image : view<image> {
  
  image(const image_data& data, optional<vec2<int>> sz) : img{data}, display_size{sz} {}
  
  auto build(auto&& b, ignore) {
    auto data_ptr = img.data();
    optional<texture_handle> texture;
    if (!img.empty())
      texture = b.context().graphics_context().create_texture(img.data(), img.size());
    auto wsize = display_size ? *display_size : img.size();
    return image_widget{{(vec2f) wsize}, texture};
  }
  
  texture_handle make_texture(application_context& ctx) {
    return ctx.graphics_context().create_texture(img.data(), img.size());
  }
  
  rebuild_result rebuild(image& old, widget_ref elem, const widget_updater& up, ignore) {
    auto& w = elem.as<image_widget>();
    if (!w.texture) {
      if (!img.empty())
        w.texture = make_texture(up.context());
    }
    else {
      up.context().graphics_context().update_texture(*w.texture, img.data(), img.size());
    }
    auto new_size = display_size ? *display_size : img.size();
    return ((vec2i)w.size() != new_size) ? (w.set_size(new_size), rebuild_result{true})
                                  : rebuild_result{};
  }
  
  const image_data& img;
  optional<vec2i> display_size;
};