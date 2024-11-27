#pragma once

#include "views_core.hpp"
#include "../util/tuple.hpp"
#include "../util/util.hpp"
#include <span>
#include <algorithm>

struct stack_data {
  float interspace = 8;
  vec2f margin;
  float align_ratio = 0;
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
  
  auto&& with_align(this auto&& self, float ratio) {
    self.info.align_ratio = ratio;
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
  bool mutated = false;
  
  [[no_unique_address]] non_copyable _;
  
  void consume(auto&& W) { 
    vec.insert( 
      vec.begin() + index++, 
      widget_box{(decltype(W)&&)(W)}
    );
    mutated = true;
  }
  
  widget_ref next() {
    return vec[index++].borrow();
  }
  
  widget_ref destroy() {
    widget_ref res = vec[index].borrow();
    to_erase.push_back(index++);
    mutated = true;
    return res;
  }
};

namespace impl {
  
  vec2f do_stack_layout(std::vector<widget_box>& children, stack_data data, int axis) {
    float pos = data.margin[axis];
    float sz = 0;
    for (auto& n : children) {
      auto child_pos = data.margin;
      child_pos[axis] = pos;
      n.set_position(child_pos);
      auto child_size = n.layout();
      pos += child_size[axis] + data.interspace;
      sz = std::max(sz, child_size[1 - axis]);
    }
    
    // Align the elements
    
    if (data.align_ratio != 0)
      for (auto& n : children) {
        vec2f child_pos;
        child_pos[axis] = n.position()[axis];
        child_pos[1 - axis] = data.margin[1 - axis] 
                              + data.align_ratio * (sz + data.margin[1 - axis] * 2) 
                              - n.size()[1 - axis] / 2;
        n.set_position(child_pos);
      } 
    
    vec2f res;
    res[axis] = pos + data.margin[axis];
    res[1 - axis] = sz + 2 * data.margin[1 - axis];
    return res;
  }
  
}

template <class T, class... Ts>
  requires (is_view_sequence<Ts> && ...)
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
    
    tuple_for_each(this->children, [&] (auto& elem) -> void {
      elem.seq_build(consumer, b, state);
    });
    
    return Res;
  }
  
  template <class S>
  rebuild_result rebuild(auto& Old, widget_ref wb, const widget_updater& up, S& state) 
  {
    auto& w = wb.as<T>();
    int index = 0;
    
    stack_updater seq_updater {
      {}, 
      up.builder(), 
      w.children_vec,
      {}, 
      index
    };
    
    rebuild_result res; 
    
    tuple_for_each_with_index( this->children, [&] (auto& elem, auto elem_index) -> void 
    { 
      res |= elem.seq_rebuild(get<elem_index.value>(Old.children), seq_updater, up, state);
    });
    
    for (auto i : seq_updater.to_erase)
      w.children_vec.erase(w.children_vec.begin() + i);
    
    if (seq_updater.mutated || (res & rebuild_result::size_change)) {
      auto old_size = wb.size();
      auto new_size = wb.layout();
      if (old_size != new_size)
        return rebuild_result::size_change;
      return {};
    }
    
    return res;
  }
  
  void destroy(widget_ref wb) {}
};

struct vstack_widget : widget_base
{
  using value_type = void;
  
  stack_data data;
  
  std::vector<widget_box> children_vec;
  
  vstack_widget(stack_data d) : data{d} {}
  
  void paint(painter& p) {
    // p.stroke_style(colors::green);
    // p.stroke_rect({0, 0}, size(), 2);
  }
  
  void on(ignore, ignore) {}
  
  bool traverse_children(auto&& fn) {
    return std::all_of(children_vec.begin(), children_vec.end(), fn);
  }
  
  auto layout() {
    return impl::do_stack_layout(children_vec, data, 1);
  }
};

template <class... Ts>
  requires (is_view_sequence<Ts> && ...)
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
  
  void paint(painter& p) {
    // p.stroke_style(colors::green);
    // p.stroke_rect({0, 0}, size(), 2);
  }
  
  void on(input_event, ignore) {}
  
  auto layout() 
  {
    return impl::do_stack_layout(children_vec, data, 0);
  }
};

template <class... Ts>
  requires (is_view_sequence<Ts> && ...)
struct hstack : stack_base<hstack_widget, Ts...>
{
  hstack(Ts... ts) : stack_base<hstack_widget, Ts...>{ts...} {}
};