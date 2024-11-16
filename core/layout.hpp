#pragma once

#include "view.hpp"
#include "ui_tree.hpp"
#include "../vec.hpp"
#include "../util/tuple.hpp"
#include "../util/ignore.hpp"
#include <span>

struct painter;

struct stack_data {
  float interspace;
  vec2f margin;
};

template <class... Ts>
struct stack 
{
  template <class Fn>
  void traverse(Fn fn) {
    apply([fn] (auto&&... args) {
      (fn(args), ...);
    }, children);
  }
  
  auto&& with_interspace(this auto&& self, float val) {
    self.info.interspace = val;
    return self;
  }
  
  auto&& with_margin(this auto&& self, vec2f margin) {
    self.info.margin = margin;
    return self;
  }
  
  tuple<Ts...> children;
  stack_data info;
};

struct stack_updater : view_sequence_updater<stack_updater> {
  widget_id this_id;
  widget_builder b;
  std::vector<widget*>& vec;
  int& index;
  
  void consume(auto&&... args) { 
    vec.insert( 
      vec.begin() + index++, 
      b.make_widget(this_id, args...) 
    );
  }
  
  widget& next() {
    return *vec[index++];
  }
  
  widget& destroy() {
    auto res = vec[index];
    vec.erase(vec.begin() + index++);
    return *res;
  }
};

template <class T, class... Ts>
struct stack_base : stack<Ts...> {
  
  template <class... Vs>
  constexpr stack_base(Vs&&... ts) : stack<Ts...>{{ts...}} {} 
  
  template <class S>
  auto build(widget_builder& b, S& state) 
  {
    auto this_id = b.next_id();
    T Res {this->info};
    auto consumer = [&] (auto&&... args) { 
      Res.children_vec.push_back(b.make_widget(this_id, args...));
    };
    
    tuple_for_each(this->children, [&] (auto& elem) {
      elem.seq_build(consumer, b, state);
    });
    
    return make_tuple(Res, empty_lens{}, widget_ctor_args{this_id, {0, 0}});
  }
  
  template <class S>
  void rebuild(auto& New, widget& wb, widget_updater& up, S& state) 
  {
    auto& w = wb.as<T>();
    int index = 0;
    
    auto next_up = up.with_parent(&wb);
    
    stack_updater seq_updater {
      {}, 
      wb.id(),
      up.builder(), 
      w.children_vec,
      index
    };
      
    tuple_for_each_with_index( this->children, [&] (auto& elem, auto elem_index) 
    { 
      elem.seq_rebuild(get<elem_index.value>(New.children), seq_updater, next_up, state);
    });
  }
};

struct vstack_widget : layout_tag 
{
  using value_type = void;
  
  stack_data data;
  
  std::vector<widget*> children_vec;
  
  vstack_widget(stack_data d) : data{d} {}
  
  void paint(painter& p, vec2f) {}
  void on(ignore, ignore) {}
  
  widget* find_child_at(vec2f pos) 
  {
    for (auto& w : children()) 
      if (w->contains(pos))
        return w;
    return {};
  }
  
  auto layout() 
  {
    float pos = data.margin.y, width = 0;
    for (auto& n : children()) {
      n->set_position(data.margin.x, pos);
      auto sz = n->layout();
      pos += sz.y + data.interspace;
      width = std::max(width, sz.x);
    }
    
    return vec2f{width, pos + data.margin.y};
  }
  
  std::span<widget*> children() {
    return {children_vec.data(), children_vec.size()};
  }
};

template <class... Ts>
struct vstack : stack_base<vstack_widget, Ts...>
{ 
  vstack(Ts... ts) : stack_base<vstack_widget, Ts...>{ts...} {}
};

/* 
struct hstack_widget : layout_tag
{
  using value_type = void;
  
  stack_data data;
  hstack_widget(stack_data d) : data{d} {}
  
  void paint(painter& p, vec2f) {}
  void on(input_event, ignore) {}
  
  auto layout( widget_tree::children_view c ) 
  {
    float pos = data.margin.x, height = 0;
    for (auto& n : c) 
    {
      n.set_position(pos, data.margin.y);
      auto sz = n.layout(c.tree());
      pos += sz.x + data.interspace;
      height = std::max(height, sz.y);
    }
    
    return vec2f{pos + data.margin.x, height + 2 * data.margin.y};
  }
};

template <class... Ts>
struct hstack : stack_base<hstack_widget, Ts...>
{
  hstack(Ts... ts) : stack_base<hstack_widget, Ts...>{ts...} {}
}; */ 