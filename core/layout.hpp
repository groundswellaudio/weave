#pragma once

#include "view.hpp"
#include "widget.hpp"
#include "../vec.hpp"
#include "../util/tuple.hpp"
#include "../util/ignore.hpp"
#include <span>
#include <algorithm>

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
  widget_builder b;
  std::vector<widget_box>& vec;
  std::vector<int> to_erase;
  int& index;
  
  void consume(auto&& W) { 
    vec.insert( 
      vec.begin() + index++, 
      widget_box{(decltype(W)&&)(W)}
    );
  }
  
  widget_ref next() {
    return vec[index++].borrow();
  }
  
  widget_ref destroy() {
    widget_ref res = vec[index].borrow();
    to_erase.push_back(index++);
    return res;
  }
};

template <class T, class... Ts>
struct stack_base : view<stack_base<T, Ts...>>, stack<Ts...> {
  
  template <class... Vs>
  constexpr stack_base(Vs&&... ts) : stack<Ts...>{{ts...}} {} 
  
  template <class S>
  auto build(const widget_builder& b, S& state) 
  {
    T Res { this->info };
    auto consumer = [&] (auto&&... args) { 
      Res.children_vec.push_back( widget_box{(decltype(args)&&)(args)...} );
    };
    
    tuple_for_each(this->children, [&] (auto& elem) {
      elem.seq_build(consumer, b, state);
    });
    
    return Res;
  }
  
  template <class S>
  void rebuild(auto& New, widget_ref wb, const widget_updater& up, S& state) 
  {
    auto& w = wb.as<T>();
    int index = 0;
    
    auto next_up = up.with_parent(wb);
    
    stack_updater seq_updater {
      {}, 
      up.builder(), 
      w.children_vec,
      {}, 
      index
    };
      
    tuple_for_each_with_index( this->children, [&] (auto& elem, auto elem_index) 
    { 
      elem.seq_rebuild(get<elem_index.value>(New.children), seq_updater, next_up, state);
    });
    
    for (auto i : seq_updater.to_erase)
      w.children_vec.erase(w.children_vec.begin() + i);
  }
};

struct vstack_widget : widget_base
{
  using value_type = void;
  
  stack_data data;
  
  std::vector<widget_box> children_vec;
  
  vstack_widget(stack_data d) : data{d} {}
  
  void paint(painter& p) {}
  void on(ignore, ignore) {}
  
  bool traverse_children(auto&& fn) {
    return std::all_of(children_vec.begin(), children_vec.end(), fn);
  }
  
  auto layout()
  {
    float pos = data.margin.y, width = 0;
    for (auto& n : children_vec) {
      n.set_position(data.margin.x, pos);
      auto sz = n.layout();
      pos += sz.y + data.interspace;
      width = std::max(width, sz.x);
    }
    
    return vec2f{width + 2 * data.margin.x, pos + data.margin.y};
  }
};

template <class... Ts>
struct vstack : stack_base<vstack_widget, Ts...>
{ 
  vstack(Ts... ts) : stack_base<vstack_widget, Ts...>{ts...} {}
};

struct hstack_widget : widget_base
{
  using value_type = void;
  
  stack_data data;
  std::vector<widget_box> children_vec;
  
  hstack_widget(stack_data d) : data{d} {}
  
  bool traverse_children(auto&& fn) {
    return std::all_of(children_vec.begin(), children_vec.end(), fn);
  }
  
  void paint(painter& p) {}
  void on(input_event, ignore) {}
  
  auto layout() 
  {
    float pos = data.margin.x, height = 0;
    for (auto& n : children_vec) 
    {
      n.set_position(pos, data.margin.y);
      auto sz = n.layout();
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
};