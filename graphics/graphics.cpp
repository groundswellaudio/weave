#include "glad/glad.h"
#include "graphics.hpp"

#define NANOVG_GL3_IMPLEMENTATION

#include "nanovg_gl.h"
#include <SDL.h>

#include "misc/default_font_embed"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

namespace weave {

graphics_context::graphics_context()
{
  if (!gladLoadGLLoader((GLADloadproc) SDL_GL_GetProcAddress))
  {
    fprintf(stderr, "failed to initialize the OpenGL context.");
    exit(1);
  }
  
  ctx = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
  
  create_font_from_memory("default", default_font_data);
  
  glClearColor(0,0,0,1);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
}

std::optional<image<rgba<unsigned char>>> decode_image(std::span<const unsigned char> data) {
  int w, h, n;
  auto img_data = stbi_load_from_memory(data.data(), data.size(), &w, &h, &n, 4);
  if (!img_data)
    return {};
  auto img = make_image_from_raw<rgba<unsigned char>>(img_data, {h, w});
  stbi_image_free(img_data);
  return img;
}

std::optional<image<rgba<unsigned char>>> load_image_from_file(const std::string& path) {
  int w, h, n;
  stbi_set_unpremultiply_on_load(1);
  stbi_convert_iphone_png_to_rgb(1);
  auto img = stbi_load(path.data(), &w, &h, &n, 4);
  if (!img) 
    return {};
  auto res = make_image_from_raw<rgba<unsigned char>>(img, {h, w});
  stbi_image_free(img);
  return res;
}

bool save_image_to_file(const std::string& path, const image<rgba<unsigned char>>& img) 
{
  auto w = img.shape()[1];
  auto h = img.shape()[0];
  
  if (path.ends_with(".jpg") || path.ends_with(".jpeg"))
    return stbi_write_jpg(path.c_str(), w, h, 4, img.data(), 80);
  
  // TODO : handle more extensions
  
  return stbi_write_png(path.c_str(), w, h, 4, img.data(), w * 4); 
}

} // weave