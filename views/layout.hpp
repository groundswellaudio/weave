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
  
  /* 
  vec2f do_stack_layout(std::vector<widget_box>& children, stack_data data, int axis, layout_context lc) {
    layout_context child_ctx;
    child_ctx.max = lc.max;
    
    float pos = data.margin[axis];
    float sz = 0;
    for (auto& n : children) {
      auto child_pos = data.margin;
      child_pos[axis] = pos;
      n.set_position(child_pos);
      auto child_size = n.layout(child_ctx);
      pos += child_size[axis] + data.interspace;
      child_ctx.max[axis] -= child_size[axis] + data.interspace;
      sz = std::max(sz, child_size[1 - axis]);
    }
    
    if (data.fill_cross_axis) {
      layout_context ctx;
      ctx.min[1 - axis] = std::max(sz, lc.min[1 - axis]);
      // Unfortunately, we have to call layout here again
      for (auto& n : children) {
        ctx.max = n.size();
        ctx.max[1 - axis] = ctx.min[1 - axis];
        n.layout(ctx);
      }
    }
    
    // Cross axis alignment 
    if (data.align_ratio != 0) {
      for (auto& n : children) {
        vec2f child_pos;
        child_pos[axis] = n.position()[axis];
        child_pos[1 - axis] = data.margin[1 - axis] 
                              + data.align_ratio * 
                                (sz - n.size()[1-axis]); // we haven't added margin to sz yet, no need to substract it
        n.set_position(child_pos);
      }
    }
    
    vec2f res;
    res[axis] = pos + data.margin[axis];
    res[1 - axis] = sz + 2 * data.margin[1 - axis];
    res = max(lc.min, res);
    return res;
  } */ 
  
  void do_stack_layout(std::vector<widget_box>& children, stack_data data, int axis, 
                        vec2f this_sz)
  {
    vec2f min {0, 0};
    float axis_expand_norm = 0;
    for (auto& e : children) {
      auto size_info = e.size_info();
      min[axis] += size_info.min[axis];
      min[axis] += data.interspace;
      min[1 - axis] = std::max(min[1 - axis], size_info.min[1 - axis]);
      axis_expand_norm += (std::min(size_info.max[axis], this_sz[axis]) - size_info.min[axis]);
    }
    
    min[axis] -= data.interspace;
    min += 2 * data.margin;

    auto layout_children = [&] (auto sizer) {
      float axis_pos = data.margin[axis];
      for (auto& e : children) {
        auto child_sz = sizer(e);
        child_sz = e.layout(child_sz);
        vec2f child_pos;
        child_pos[axis] = axis_pos;
        child_pos[1 - axis] = data.margin[1 - axis];
        e.set_position(child_pos);
        axis_pos += child_sz[axis];
        axis_pos += data.interspace;
      }
    };
    
    auto cross_axis_size = [data, this_sz, axis] (widget_size_info info) {
      // auto cross_axis_expand = info.expand_factor[1 - axis];
      if (data.fill_cross_axis)
        return this_sz[1 - axis];
      else
        return std::min(this_sz[1 - axis], info.max[1 - axis]);
    };
    
    auto axis_leftover = this_sz[axis] - min[axis];
    if (axis_leftover == 0 || axis_expand_norm == 0) {
      layout_children( [&] (auto& e) {
        auto info = e.size_info();
        vec2f child_size = info.min;
        child_size[1 - axis] = cross_axis_size(info);
        return child_size;
      });
    }
    else {
      // Divide the difference between size[axis] and min[axis] according to their expand 
      // factor
      layout_children( [&] (auto& e) {
        auto info = e.size_info();
        vec2f child_size = info.min;
        child_size[axis] += (std::min(info.max[axis], this_sz[axis]) - info.min[axis]) * axis_leftover / axis_expand_norm;
        child_size[1 - axis] = cross_axis_size(info);
        return child_size;
      });
    }
    
    // Cross axis alignment 
    if (data.align_ratio != 0) {
      for (auto& n : children) {
        vec2f child_pos;
        child_pos[axis] = n.position()[axis];
        child_pos[1 - axis] = data.margin[1 - axis] 
                              + data.align_ratio * 
                                (this_sz[1 - axis] - n.size()[1-axis] - 2 * data.margin[1 - axis]); 
        n.set_position(child_pos);
      }
    }
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
  layout_context old_bc;
  
  stack(stack_data d) : data{d} {}
  
  void paint(painter& p) {
    p.fill_style(data.background_col);
    p.fill(rectangle(size()));
    // p.stroke_style(colors::green);
    //  p.stroke(rectangle(size()));
  }
  
  widget_size_info size_info() const {
    if (!children_vec.size())
      return { data.margin * 2, data.margin * 2 };
    
    point min {0, 0}, max{0, 0};
    // vec2f factor {0, 0};
    for (auto& e : children_vec) {
      auto i = e.size_info();
      min[Axis] += i.min[Axis];
      min[1 - Axis] = std::max(min[1 - Axis], i.min[1 - Axis]);
      max[Axis] += i.max[Axis];
      max[1 - Axis] = std::max(max[1 - Axis], i.max[1 - Axis]);
      /* 
      factor[Axis] += i.expand_factor[Axis];
      factor[1 - Axis] = std::max(i.expand_factor[1 - Axis], factor[1 - Axis]); */ 
    }
    
    min[Axis] += (children_vec.size() - 1) * data.interspace;
    max[Axis] += (children_vec.size() - 1) * data.interspace;
    min += data.margin * 2.f;
    max += data.margin * 2.f;
    return {min, max};
  }
  
  void on(ignore, ignore) {}
  
  bool traverse_children(auto&& fn) {
    return std::all_of(children_vec.begin(), children_vec.end(), fn);
  }
  
  auto layout(point sz) {
    impl::do_stack_layout(children_vec, data, Axis, sz);
    return sz;
  }
};

using hstack = stack<0>;
using vstack = stack<1>;

struct flow : widget_base {
  
  widget_size_info size_info() const {
    return {{200, 200}};
  }
  
  point layout(point sz) {
    point pos = {0, 0};
    float row_h = 0;
    
    widget_size_info row_sz_info;
    
    for (auto& c : children_vec) {
      auto i = c.size_info();
      row_sz_info.min[0] += i.min[0];
      row_sz_info.min[1] = std::max(row_sz_info.min[1], i.min[1]);
      
    }
    
    /* 
    for (auto& c : children_vec) {
      auto sz = c.layout(lc);
      if (pos.x + sz.x > size().x) {
        pos.x = 0;
        pos.y += row_h;
        row_h = sz.y;
        c.set_position(pos);
      }
      else {
        c.set_position(pos);
        row_h = std::max(row_h, sz.y);
        pos.x += sz.x;
      }
    }
    
    return {size().x, pos.y + row_h}; */ 
    return sz;
  }
  
  auto traverse_children(auto&& fn) {
    return std::all_of(children_vec.begin(), children_vec.end(), fn);
  }
  
  void on(ignore, ignore) {}
  void paint(painter&) {}
  
  std::vector<widget_box> children_vec;
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
  
  void destroy(widget_ref wb) {}
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