#pragma once

#include "view.hpp"
#include "../vec.hpp"
#include "../util/tuple.hpp"

struct painter;

template <class... Ts>
struct stack : composed_view {
  template <class Fn>
  void traverse(Fn fn) {
    apply([fn] (auto&&... args) {
      (fn(args), ...);
    }, children);
  }
  
  tuple<Ts...> children;
};

struct vstack_widget : layout_tag 
{
  void paint(painter& p, vec2f) {}
  void on(input_event) {}
  
  auto layout( widget_tree::children_view c ) 
  {
    float pos = 0, width = 0;
    for (auto& n : c) {
      n.set_pos(0, pos);
      auto sz = n.layout(c.tree());
      pos += sz.y;
      width = std::max(width, sz.x);
    }
    
    return vec2f{width, pos};
  }
};

template <class... Ts>
struct vstack : stack<Ts...>
{ 
  vstack(Ts... ts) : stack<Ts...>{{}, {ts...}} {}
  
  template <class Ctx>
  auto construct(Ctx ctx) {
    return ctx.template create_widget<vstack_widget>(this->view_id, vec2f{0, 0}); 
  }
};

struct hstack_widget : layout_tag 
{
  void paint(painter& p, vec2f) {}
  void on(input_event) {}
  
  auto layout( widget_tree::children_view c ) 
  {
    float pos = 0, height = 0;
    for (auto& n : c) 
    {
      n.set_pos(pos, 0);
      auto sz = n.layout(c.tree());
      pos += sz.x;
      height = std::max(height, sz.y);
    }
    
    return vec2f{pos, height};
  }
};

template <class... Ts>
struct hstack : stack<Ts...>
{
  hstack(Ts... ts) : stack<Ts...>{{}, {ts...}} {}
  
  template <class Ctx>
  auto construct(Ctx ctx) {
    return ctx.template create_widget<hstack_widget>(this->view_id, vec2f{0, 0}); 
  }
};