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
  
  image(const image_data& data, vec2<int> sz) : img{data}, size{sz} {}
  
  auto build(auto&& b, ignore) {
    auto data_ptr = img.data();
    optional<texture_handle> texture;
    if (size != vec2i{0, 0})
      texture = b.context().graphics_context().create_texture(img.data(), size);
    return image_widget{{size}, texture};
  }
  
  texture_handle make_texture(application_context& ctx) {
    return ctx.graphics_context().create_texture(img.data(), size);
  }
  
  rebuild_result rebuild(image& old, widget_ref elem, const widget_updater& up, ignore) {
    auto& w = elem.as<image_widget>();
    if (!w.texture) {
      if (img.size() != vec2i{0, 0})
        w.texture = make_texture(up.context());
    }
    else {
      up.context().graphics_context().update_texture(*w.texture, img.data(), size);
      w.set_size(size);
    }
    return {};
  }
  
  const image_data& img;
  vec2i size;
};