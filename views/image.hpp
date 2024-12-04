#pragma once

#include "../util/optional.hpp"

struct image_widget : widget_base {
  
  void on(ignore, ignore) {}
  
  void paint(painter& p) {
    if (!texture)
      return;
    p.fill_style(*texture, -corner_offset, size());
    p.rectangle({0, 0}, size());
    p.stroke_style(colors::white);
    p.stroke_rect({0, 0}, size());
  }
  
  optional<texture_handle> texture;
  vec2i corner_offset;
};

namespace views {

static constexpr auto default_image_proj = [] (auto&& pix) { return pix; };

template <class ImgT, class Proj = decltype(default_image_proj)>
struct image : view<image<ImgT, Proj>> {
  
  //using ImgT = ::image<rgba<unsigned char>>;
  image(const ImgT& data, bool RefreshWhen, Proj proj = {}) 
  : img{data}, refresh{RefreshWhen}, image_proj{proj} 
  {
  }
  
  auto& with_corner_offset(vec2i corner) {
    corner_offset = corner;
    return *this;
  }
  
  vec2f get_display_size() const {
    if (img.empty())
      return {0, 0};
    // CAREFUL here : by convention the dimensions of images are stored as (y, x), but the size of widget is (x, y)!
    auto img_size = vec2f{(float)img.shape()[1], (float)img.shape()[0]};
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
  
  auto build(auto&& b, ignore) {
    optional<texture_handle> texture;
    if (!img.empty() && refresh)
      texture = make_texture(b.context());
    auto wsize = get_display_size();
    return image_widget{{wsize}, texture, corner_offset};
  }
  
  rebuild_result rebuild(image& old, widget_ref elem, const widget_updater& up, ignore) {
    auto& w = elem.as<image_widget>();
    if (!w.texture) {
      if (!img.empty())
        w.texture = make_texture(up.context());
    }
    else if (refresh) {
      std::cout << "refreshing image " << std::endl;
      auto& gctx = up.context().graphics_context();
      gctx.delete_texture(*w.texture);
      
      if (img.empty()) {
        w.texture = std::nullopt;
        w.set_size({0, 0});
        return rebuild_result::size_change;
      }
      
      w.texture = make_texture(up.context());
    }
    auto new_size = get_display_size();
    if (w.size() == new_size)
      return {};
    w.set_size(new_size);
    return rebuild_result::size_change;
  }
  
  private : 
  
  texture_handle make_texture(application_context& ctx) {
    if constexpr (^ImgT == ^image<rgba<unsigned char>>)
      return ctx.graphics_context().create_texture(img, img.shape());
    else {
      ::image<rgba<unsigned char>> tex;
      tex.reshape(img.shape());
      for (int y : iota(tex.shape()[0]))
        for (int x : iota(tex.shape()[1]))
          tex(y, x) = static_cast<rgba<unsigned char>>(image_proj(img(y, x)));
      return ctx.graphics_context().create_texture(tex, tex.shape());
    }
  }
  
  const ImgT& img;
  Proj image_proj;
  optional<vec2f> max_bounds;
  vec2i corner_offset = {0, 0};
  bool refresh; 
};

} // views