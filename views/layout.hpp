#pragma once

#include "views_core.hpp"
#include "../util/tuple.hpp"
#include "../util/util.hpp"
#include <span>
#include <algorithm>
#include <concepts>

namespace weave {

struct stack_data {
  float interspace = 8;
  point margin {0, 0};
  float align_ratio = 0;
  rgba_u8 background_col {0, 0, 0, 0};
  bool fill_cross_axis = false;
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
  
  auto&& interspace(this auto&& self, float val) {
    self.info.interspace = val;
    return self;
  }
  
  auto&& margin(this auto&& self, vec2f margin) {
    self.info.margin = margin;
    return self;
  }
  
  auto&& align(this auto&& self, float ratio) {
    self.info.align_ratio = ratio;
    return self;
  }
  
  auto&& align_center(this auto&& self) {
    self.info.align_ratio = 0.5;
    return self;
  }
  
  auto&& background(this auto&& self, rgba_u8 col) {
    self.info.background_col = col;
    return self;
  }
  
  auto&& fill(this auto&& self) {
    self.info.fill_cross_axis = true;
    return self;
  }
  
  tuple<Ts...> children;
  stack_data info;
};

struct stack_updater : view_sequence_updater<stack_updater> {
  build_context b;
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
  
  // Called after the sizing has been done
  void place_stack_layout_elements(std::span<widget_box> children, stack_data data, int axis, 
                                   point this_size)
  {
    float axis_pos = data.margin[axis];
    
    for (auto& c : children)
    {
      point pos;
      pos[axis] = axis_pos;
      pos[1 - axis] = data.margin[1 - axis] + data.align_ratio * 
                                (this_size[1-axis] - c.size()[1-axis] - 2 * data.margin[1 - axis]);
      c.set_position(pos);
      
      axis_pos += c.size()[axis];
      axis_pos += data.interspace;
    }
    
    axis_pos -= data.interspace;
    axis_pos += data.margin[axis];
    
    //assert( axis_pos == this_size[axis] && "incoherent final size in stack layout" );
  }
  
  void resolve_max_constraints(std::span<widget_box> children, 
                              const std::vector<widget_size_info> sz_infos, int axis, 
                              point this_size,
                              stack_data data)
  {
    while (true)
    {
      float remaining_space = 0;
      std::vector<widget_ref> unconstrained;
      
      for (auto i : iota(children.size())) 
      {
        auto sz = children[i].size();
        auto sz_axis = sz[axis];
        auto sz_max_axis = sz_infos[i].max[axis];
        if (sz_axis >= sz_max_axis) {
          remaining_space += (sz_axis - sz_max_axis);
          auto sz = children[i].size();
          sz[axis] = sz_max_axis; 
          children[i].set_size(sz);
        }
        // If we arrive at a conflict with the aspect ratio, modify first the cross axis size to fit, then the axis size if needed
        else if (sz_infos[i].aspect_ratio && *sz_infos[i].aspect_ratio * sz.x != sz.y) {
          auto old_sz_axis = sz_axis;
          if (axis == 0)
            sz.y = sz.x * *sz_infos[i].aspect_ratio;
          else
            sz.x = sz.y / *sz_infos[i].aspect_ratio;
          if (sz[1 - axis] > sz_infos[i].max[1 - axis]
              || sz[1 - axis] > this_size[1 - axis] - 2 * data.margin[1 - axis])
          {
            sz[1 - axis] = std::min(sz_infos[i].max[1 - axis], this_size[1 - axis] - 2 * data.margin[1 - axis]);
            if (axis == 0)
              sz.x = sz.y / *sz_infos[i].aspect_ratio;
            else
              sz.y = sz.x * *sz_infos[i].aspect_ratio;
          }
          else if (sz[1 - axis] < sz_infos[i].min[1 - axis]) {
            sz[1 - axis] = sz_infos[i].min[1 - axis];
            if (axis == 0)
              sz.x = sz.y / *sz_infos[i].aspect_ratio;
            else
              sz.y = sz.x * *sz_infos[i].aspect_ratio;
          }
          children[i].set_size(sz);
          remaining_space += old_sz_axis - sz[axis]; 
        }
        else
          unconstrained.push_back(children[i].borrow());
      }
      
      if (remaining_space == 0)
        return;
      
      float sum_unconstrained_flex = 0;
      
      for (auto u : unconstrained)
        sum_unconstrained_flex += u.size_info().flex_factor[axis];
        
      for (auto u : unconstrained) {
        auto sz = u.size();
        auto szi = u.size_info();
        sz[axis] += remaining_space * szi.flex_factor[axis] / sum_unconstrained_flex;
        sz[axis] = std::min(szi.max[axis], sz[axis]);
        u.set_size(sz);
      }
    }
  }
  
  void resolve_min_constraints(std::span<widget_box> children, 
                              const std::vector<widget_size_info>& sz_infos, int axis, 
                              point this_size)
  {
    while (true)
    {
      float space_to_remove = 0;
      
      std::vector<widget_ref> unconstrained;
      
      for (auto i : iota(children.size())) 
      {
        auto sz_axis = children[i].size()[axis];
        auto sz_min_axis = sz_infos[i].min[axis];
        if (sz_axis < sz_min_axis) {
          space_to_remove += (sz_min_axis - sz_axis);
          auto sz = children[i].size();
          sz[axis] = sz_min_axis; 
          children[i].set_size(sz);
        }
        else
          unconstrained.push_back(children[i].borrow());
      }
      
      if (space_to_remove == 0)
        return;
      
      float sum_unconstrained_inv_flex = 0;
      
      for (auto u : unconstrained)
        sum_unconstrained_inv_flex += 1.f / u.size_info().flex_factor[axis];
        
      for (auto u : unconstrained) {
        auto sz = u.size();
        sz[axis] -= space_to_remove / (u.size_info().flex_factor[axis] * sum_unconstrained_inv_flex);
        u.set_size(sz);
      }
    }
  }
  
  void stack_layout(std::span<widget_box> children, stack_data data, int axis, 
                    point this_size)
  {
    std::vector<widget_size_info> sz_infos;
    for (auto& c : children)
      sz_infos.push_back(c.size_info());
    
    float sum_nominal = 0;
    
    for (auto szi : sz_infos)
    {
      sum_nominal += szi.nominal[axis];
    }
    
    sum_nominal += data.interspace * (children.size() - 1);
    sum_nominal += data.margin[axis] * 2;
    
    auto size_from_axis = [&] (int i, float sz_axis) -> point {
      auto szi = sz_infos[i];
      auto cross_axis_size =  
        szi.aspect_ratio ? (axis == 0 ? sz_axis / *szi.aspect_ratio 
                                      : sz_axis * *szi.aspect_ratio)
                         :
             szi.flex_factor[1 - axis] != 0 
               ? (this_size[1 - axis] - 2 * data.margin[1 - axis])
               : szi.nominal[1 - axis];
      
      // If we arrive at a conflict with the maximum and aspect ratio, adjust axis size 
      if (szi.aspect_ratio 
          && (cross_axis_size > this_size[1 - axis] - data.margin[1 - axis] * 2
          || cross_axis_size > szi.max[1 - axis]))
      {
        cross_axis_size = std::min(this_size[1 - axis] - data.margin[1 - axis] * 2, szi.max[1 - axis]);
        sz_axis = axis ? cross_axis_size * *szi.aspect_ratio : cross_axis_size / *szi.aspect_ratio;
      }
      
      point res;
      res[axis] = sz_axis;
      res[1 - axis] = cross_axis_size;
      return res;
    };
    
    if (sum_nominal < this_size[axis]) 
    {
      auto remaining_space = this_size[axis] - sum_nominal;
      auto sum_flex = 0.f; 
      for (auto szi : sz_infos)
        sum_flex += szi.flex_factor[axis];
      
      if (sum_flex != 0.f)
      {
        for (auto i : iota(children.size())) 
        {
          auto axis_sz = sz_infos[i].nominal[axis] 
                            + remaining_space * sz_infos[i].flex_factor[axis] / sum_flex;
          point sz = size_from_axis(i, axis_sz);
          children[i].set_size(sz);
        }
      }
      else 
      {
        // None of the children are extensible, just set them to nominal size
        for (auto i : iota(children.size())) {
          point sz = size_from_axis(i, sz_infos[i].nominal[axis]);
          children[i].set_size(sz);
        }
      }
      
      resolve_max_constraints(children, sz_infos, axis, this_size, data);
    }
    else 
    {
      auto space_to_remove = sum_nominal - this_size[axis];
      float sum_inv_flex = 0;
      
      for (auto szi : sz_infos)
        if (szi.flex_factor[axis] > 0.f)
          sum_inv_flex += 1.f / szi.flex_factor[axis];
      
      for (auto i : iota(children.size())) 
      {   
        auto axis_sz =
          sz_infos[i].flex_factor[axis] == 0.f ? sz_infos[i].nominal[axis]
                                               : 
          sz_infos[i].nominal[axis] 
            - space_to_remove * 1.f / (sz_infos[i].flex_factor[axis] * sum_inv_flex);
        axis_sz = std::max(sz_infos[i].min[axis], axis_sz);
        point sz = size_from_axis(i, axis_sz);
        children[i].set_size(sz);
      }
      
      resolve_min_constraints(children, sz_infos, axis, this_size);
    }
    
    place_stack_layout_elements(children, data, axis, this_size);
    
    for (auto& c : children)
      c.layout(c.size());
  }
  
  template <class T, class V, class S>
  auto build_stack(V& view, const build_context& ctx, S& state, auto&&... ctor_args) {
    T Res { ctor_args... };
    auto consumer = [&] (auto&&... args) { 
      Res.children_vec.push_back( widget_box{(decltype(args)&&)(args)...} );
    };
    
    tuple_for_each([&] (auto& elem) {
      elem.seq_build(consumer, ctx, state);
    }, view.children);
    return Res;
  }
  
  template <class T, class V, class S>
  rebuild_result rebuild_stack(V& New, V& Old, widget_ref wb, const build_context& ctx, S& state) {
    auto& w = wb.as<T>();
    int index = 0;
    
    stack_updater seq_updater {
      {}, 
      ctx, 
      w.children_vec,
      {}, 
      index
    };
    
    rebuild_result res; 
    
    tuple_for_each_with_index( [&] (auto elem_index, auto& elem) -> void 
    { 
      res |= elem.seq_rebuild(get<elem_index.value>(Old.children), seq_updater, ctx, state);
    }, New.children);
    
    // FIXME : this could be rewritten more efficiently with erase(remove(...))
    for (auto i : seq_updater.to_erase)
      w.children_vec.erase(w.children_vec.begin() + i);
    
    if (seq_updater.mutated || (res & rebuild_result::size_change)) {
      w.do_layout(w.size());
      return {};
    }
    
    return res;
  }
  
} // impl

namespace widgets {

template <int Axis>
struct stack : widget_base
{
  stack_data data;
  std::vector<widget_box> children_vec;
  
  stack(stack_data d) : data{d} {}
  
  void paint(painter& p) {
    p.fill_style(data.background_col);
    p.fill(rectangle(size()));
    // p.stroke_style(colors::green);
    //  p.stroke(rectangle(size()));
  }
  
  auto size_info() const {
    widget_size_info res;
    point min {0, 0}, max{0, 0}, nominal_size{0, 0};
    
    res.flex_factor = point{0, 0};
    
    if (!children_vec.size()) {
      return widget_size_info{min, max, nominal_size};
    }
    
    for (auto& e : children_vec) {
      auto i = e.size_info();
      res.min[Axis] += i.min[Axis];
      res.min[1 - Axis] = std::max(res.min[1 - Axis], i.min[1 - Axis]);
      res.max[Axis] += i.max[Axis];
      res.max[1 - Axis] = std::max(res.max[1 - Axis], i.max[1 - Axis]);
      res.nominal[Axis] += i.nominal[Axis];
      
      res.flex_factor += i.flex_factor;
      
      res.nominal[1 - Axis] = std::max(res.nominal[1 - Axis], 
                                            i.nominal[1 - Axis]);
    }
    
    res.min[Axis] += (children_vec.size() - 1) * data.interspace;
    res.max[Axis] += (children_vec.size() - 1) * data.interspace;
    min += data.margin * 2.f;
    max += data.margin * 2.f;
    res.nominal[Axis] += data.margin[Axis] * 2.f + (children_vec.size() - 1) * data.interspace;
    res.nominal[1 - Axis] += data.margin[1 - Axis] * 2.f;
    // Average the flex factor cross axis
    res.flex_factor[1 - Axis] /= children_vec.size();
    return res;
  }
  
  void on(ignore, ignore) {}
  
  bool traverse_children(auto&& fn) {
    return std::all_of(children_vec.begin(), children_vec.end(), fn);
  }
  
  auto layout(point sz) {
    impl::stack_layout(children_vec, data, Axis, sz);
    return sz;
  }
};

using hstack = stack<0>;
using vstack = stack<1>;

struct flow : widget_base {
  
  auto size_info() const {
    widget_size_info res;
    res.min = nominal_size / 2; 
    res.nominal = nominal_size;
    res.flex_factor = flex_factor;
    return res;
  }
  
  void layout(point sz) 
  {
    auto row_begin = children_vec.begin();
    auto row_end = children_vec.begin();
    float p = margin.x;
    float y = 0;
    
    float row_height = 0;
    
    while (true) {
      auto i = row_end->size_info();
      auto next = p;
      next += interspace.x;
      next += i.nominal.x;
      row_height = std::max(i.nominal.y, row_height);
      
      auto do_row_layout = [&] () {
        stack_data stack_prop;
        stack_prop.interspace = interspace.x;
        stack_prop.margin = margin;
        impl::stack_layout(std::span{row_begin, row_end}, stack_prop, 0, point{sz.x, row_height});
        for (auto ic = row_begin; ic != row_end; ++ic) 
          ic->set_position(ic->position() + point{0, y});
      };
      
      if (i.aspect_ratio) 
        row_height = std::max(i.nominal.x / *i.aspect_ratio, row_height);
      
      if (next + margin.x > sz.x) {
        do_row_layout();
        p = margin.x;
        row_begin = row_end;
        y += row_height + interspace.y;
        row_height = 0;
      }
      else {
        p = next;
        ++row_end;
        if (row_end == children_vec.end()) {
          do_row_layout();
          return;
        }
      }
    }
  }
  
  auto traverse_children(auto&& fn) {
    return std::all_of(children_vec.begin(), children_vec.end(), fn);
  }
  
  void on(ignore, ignore) {}
  void paint(painter&) {}
  
  std::vector<widget_box> children_vec;
  point nominal_size = {300, 300};
  point interspace = {5, 5};
  point margin = {5, 5};
  point flex_factor = {1, 1};
};

} // widgets

namespace views 
{

template <class T, class... Ts>
struct stack_base : view<stack_base<T, Ts...>>, stack<Ts...> {
  
  template <class... Vs>
  constexpr stack_base(Vs&&... ts) : stack<Ts...>{ {(Vs&&)(ts)...} } {} 
  
  stack_base(stack_base&& o) : stack<Ts...>{std::move(o)} {}
  stack_base(const stack_base&) = default;
  
  template <class S>
  auto build(const build_context& ctx, S& state) 
  {
    auto Res = impl::build_stack<T>(*this, ctx, state, this->info);
    return Res;
  }
  
  template <class S>
  rebuild_result rebuild(stack_base<T, Ts...>& Old, widget_ref wb, const build_context& ctx, S& state) {
    return impl::rebuild_stack<T>(*this, Old, wb, ctx, state);
  }
};

template <class... Ts>
  requires (is_view_sequence<Ts> && ...)
struct vstack : stack_base<widgets::stack<1>, Ts...>
{ 
  template <class... Vs>
    requires (std::constructible_from<Ts, Vs&&> && ...)
  vstack(Vs&&... ts) : stack_base<widgets::vstack, Ts...>{(Vs&&)ts...} {}
  vstack(vstack&&) = default;
  vstack(const vstack&) = default;
  vstack& operator=(vstack&&) = default;
  vstack& operator=(const vstack&) = default;
};

template <class... Ts>
vstack(Ts...) -> vstack<Ts...>;

template <class... Ts>
  requires (is_view_sequence<Ts> && ...)
struct hstack : stack_base<widgets::stack<0>, Ts...>
{ 
  template <class... Vs>
    requires (std::constructible_from<Ts, Vs&&> && ...)
  hstack(Vs&&... ts) : stack_base<widgets::hstack, Ts...>{(Vs&&)ts...} {}
  hstack(hstack&&) = default;
  hstack(const hstack&) = default;
  hstack& operator=(hstack&&) = default;
  hstack& operator=(const hstack&) = default;
};

template <class... Ts>
hstack(Ts...) -> hstack<Ts...>;

template <class... Ts>
struct flow : view<flow<Ts...>> {
  
  template <class... Vs>
    requires (std::constructible_from<Ts, Vs&&> && ...)
  flow(float width, Vs&&... ts) 
  : width{width}, children{(Vs&&)ts...} {}
  
  auto build(auto&& ctx, auto& state) {
    auto res = impl::build_stack<widgets::flow>(*this, ctx, state, vec2f{width, 0});
    return res;
  }
  
  auto rebuild(flow<Ts...>& Old, widget_ref w, auto&& ctx, auto& state) {
    return impl::rebuild_stack<widgets::flow>(*this, Old, w, ctx, state);
  }
  
  float width;
  tuple<Ts...> children;
};

template <class... Ts>
flow(float, Ts... ts) -> flow<Ts...>;  

} // views

} // weave