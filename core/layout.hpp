#pragma once

#include "view.hpp"
#include "ui_tree.hpp"
#include "../vec.hpp"
#include "../util/tuple.hpp"
#include "../util/ignore.hpp"

struct painter;

template <class... Ts>
struct stack : view
{
  template <class Fn>
  void traverse(Fn fn) {
    apply([fn] (auto&&... args) {
      (fn(args), ...);
    }, children);
  }
  
  tuple<Ts...> children;
};

template <class T, class... Ts>
struct stack_base : stack<Ts...> {
  
  template <class... Vs>
  constexpr stack_base(Vs&&... ts) : stack<Ts...>{{}, {ts...}} {} 
  
  template <class S>
  void construct(widget_tree_builder& b, S& state) 
  {
    auto next_b = b.template create_widget<T>(empty_lens{}, {0, 0});
    tuple_for_each(this->children, [&next_b, &state] (auto& elem) {
      elem.construct(next_b, state);
    });
  }
  
  template <class S>
  void rebuild(stack_base<T, Ts...>& New, widget_tree_updater& b, S& state) {
    auto [w, next_updater] = b.consume_parent();
    
    tuple_for_each_with_index( this->children, [&next_updater, &state, &New] (auto& elem, auto index) 
    {
      elem.rebuild(get<index.value>(New.children), next_updater, state);
    });
  }
};

struct vstack_widget : layout_tag 
{
  void paint(painter& p, vec2f) {}
  void on(input_event, ignore, ignore) {}
  
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
struct vstack : stack_base<vstack_widget, Ts...>
{ 
  vstack(Ts... ts) : stack_base<vstack_widget, Ts...>{ts...} {}
};

struct hstack_widget : layout_tag
{
  void paint(painter& p, vec2f) {}
  void on(input_event, ignore, ignore) {}
  
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
struct hstack : stack_base<hstack_widget, Ts...>
{
  hstack(Ts... ts) : stack_base<hstack_widget, Ts...>{ts...} {}
};