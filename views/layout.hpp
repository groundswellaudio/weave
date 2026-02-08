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
  void place_stack_layout_elements(std::vector<widget_box>& children, stack_data data, int axis, 
                                  vec2f this_size)
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
  
  void stack_layout(std::vector<widget_box>& children, stack_data data, int axis, 
                        vec2f this_size)
  {
    // 2 cases : 
    //   - the nominal size of all widgets is below the total size, -> we have extra space to allocate : let ES be that space
    //   - the nominal size is above the total size -> we have to decide which widgets to shrink, let ES be abs(SUM(NS) - total_size)
    
    // First case : 
    //  for each widget w : 
    // three case : 1/ it can be extended and use the extra space (e.g. length of a slider), 
    //              2/ it can be extended but the extra space past the nominal size is not needed (e.g. width of a text button)
    //              3/ it cannot be extended past the nominal size 
    // let S(1) be the set of widget in the first category, if not empty, then let RS = ES and for each w in S(1)
    // assign s.size[axis] <= min(w.nominal_size[axis] + ES[axis] / size(S(1)), w.max_size[axis])
    // assign RS -= (w.size[axis] - w.nominal_size[axis]) 
    
    // if after that RS > 0 and S(2) is not empty, then divide RS evenly between S(2)
    
    // Second case : 
    // for each widget w : 
    //   three case : 1/ it can be shrunk without losing display information (e.g. the length of a slider)
    //                2/ it can be shrunk with some loss of information (e.g. textbox)
    //                3/ it cannot be shrunk past the nominal size
    
    // let RS = ES, for each w in S(1) do : 
    // w.size[axis] <= max(w.nominal_size[axis] - ES / size(S(1)), w.min_size[axis])
    // RS -= w.nominal_size[axis] - w.size[axis]
    
    // if after that RS > 0 and S(2) is not empty, do for each w in S(2)
    // w.size[axis] <= max(w.nominal_size[axis] - RS / size(S(2)), w.min_size[axis])
    
    std::vector<widget_size_info> size_infos; 
    for (auto& c : children)
      size_infos.push_back(c.size_info());
    
    auto nominal_size_axis = data.margin[axis] * 2 + data.interspace * (children.size() - 1);
    for (auto& si : size_infos)
      nominal_size_axis += si.nominal_size[axis];
    
    auto cross_axis_size = [this_size, axis, data] (widget_size_info i) {
      if (data.fill_cross_axis && !i.policy[1 - axis].is_not_expansible())
        return std::min(i.max[1 - axis], this_size[1 - axis] - 2 * data.margin[1 - axis]);
      if (i.policy[1 - axis].is_expansion_neutral() || i.policy[1 - axis].is_not_expansible())
        return i.nominal_size[1 - axis];
      return std::min(i.max[1 - axis], this_size[1 - axis] - 2 * data.margin[1 - axis]);
    };
    
    if (nominal_size_axis < this_size[axis]) 
    {
      // we have extra space to allocate
      auto extra_space = this_size[axis] - nominal_size_axis;
      
      std::vector<widget_ref> s1;
      for (auto k : iota(size_infos.size()))
        if (size_infos[k].policy[axis].is_usefully_expansible())
          s1.push_back(children[k].borrow());
      
      auto rs = extra_space;
      for (auto w : s1) 
      {
        vec2f w_size;
        auto size_info = w.size_info();
        w_size[axis] = std::min(size_info.nominal_size[axis] + extra_space / s1.size(), size_info.max[axis]);
        w_size[1 - axis] = cross_axis_size(size_info);
        w.set_size(w_size);
        rs -= (w_size[axis] - size_info.nominal_size[axis]);
      }
      
      std::vector<widget_ref> s2;
      for (auto k : iota(size_infos.size()))
        if (size_infos[k].policy[axis].is_expansion_neutral())
          s2.push_back(children[k].borrow());
      
      for (auto w : s2)
      {
        vec2f w_size;
        auto size_info = w.size_info();
        w_size[axis] = std::min(size_info.nominal_size[axis] + rs / s2.size(), size_info.max[axis]);
        w_size[1 - axis] = cross_axis_size(size_info); 
        w.set_size(w_size);
      }
    }
    else 
    {
      auto space_to_remove = nominal_size_axis - this_size[axis];
      auto rs = space_to_remove;
      
      int s1_size = 0;
      
      for (auto k : iota(size_infos.size()))
        if (size_infos[k].policy[axis].is_losslessly_shrinkable())
          ++s1_size;
          
      for (auto k : iota(size_infos.size()))
      { 
        if (!size_infos[k].policy[axis].is_losslessly_shrinkable())
          continue;
        auto w = children[k].borrow();
        auto size_info = size_infos[k];
        vec2f w_size;
        w_size[axis] = std::max(size_info.nominal_size[axis] - space_to_remove / s1_size, size_info.min[axis]);
        w_size[1 - axis] = cross_axis_size(size_info);
        w.set_size(w_size);
        rs -= (size_info.nominal_size[axis] - w_size[axis]);
      }
      
      int s2_size = 0;
      
      for (auto k : iota(size_infos.size())) 
        if (size_infos[k].policy[axis].is_lossily_shrinkable())
          ++s2_size;
          
      for (auto k : iota(size_infos.size())) 
      {
        if (!size_infos[k].policy[axis].is_lossily_shrinkable())
          continue;
        auto w = children[k].borrow();
        vec2f w_size;
        auto size_info = size_infos[k];
        w_size[axis] = std::max(size_info.nominal_size[axis] - rs / s2_size, size_info.min[axis]);
        w_size[1 - axis] = cross_axis_size(size_info);
          w.set_size(w_size);
      }
      
      for (int k : iota(size_infos.size())) 
      {
        if (!size_infos[k].policy[axis].is_not_shrinkable())
          continue;
        vec2f w_size;
        auto size_info = size_infos[k];
        w_size[axis] = size_info.nominal_size[axis];
        w_size[1 - axis] = cross_axis_size(size_info);
        children[k].set_size(w_size);
      }
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
  
  widget_size_info size_info() const {
    size_policy sp {size_policy::not_shrinkable, size_policy::not_expansible};
    vec2<size_policy> policy {sp, sp};
    
    point min {0, 0}, max{0, 0}, nominal_size{0, 0};
    
    if (!children_vec.size()) {
      return widget_size_info{policy, min, max, nominal_size};
    }
      
    for (auto& e : children_vec) {
      auto i = e.size_info();
      min[Axis] += i.min[Axis];
      min[1 - Axis] = std::max(min[1 - Axis], i.min[1 - Axis]);
      max[Axis] += i.max[Axis];
      max[1 - Axis] = std::max(max[1 - Axis], i.max[1 - Axis]);
      nominal_size[Axis] += i.nominal_size[Axis];
      
      if (i.nominal_size[1 - Axis] > nominal_size[1 - Axis]) {
        nominal_size[1 - Axis] = i.nominal_size[1 - Axis];
        policy[1 - Axis] = i.policy[1 - Axis];
      }
      
      if (i.policy[Axis].is_lossily_shrinkable() && policy[Axis].shrink == size_policy::not_shrinkable)
        policy[Axis].shrink = size_policy::lossily_shrinkable;
      else if (i.policy[Axis].is_losslessly_shrinkable())
        policy[Axis].shrink = size_policy::losslessly_shrinkable;
      
      if (i.policy[Axis].is_expansion_neutral() && policy[Axis].expand == size_policy::not_expansible)
        policy[Axis].expand = size_policy::expansion_neutral;
      else if (i.policy[Axis].is_usefully_expansible())
        policy[Axis].expand = size_policy::usefully_expansible;
    }
    
    min[Axis] += (children_vec.size() - 1) * data.interspace;
    max[Axis] += (children_vec.size() - 1) * data.interspace;
    min += data.margin * 2.f;
    max += data.margin * 2.f;
    nominal_size += data.margin * 2.f + (children_vec.size() - 1) * data.interspace;
    return widget_size_info{policy, min, max, nominal_size};
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
  
  widget_size_info size_info() const {
    size_policy sp {size_policy::lossily_shrinkable, size_policy::usefully_expansible};
    widget_size_info res {{sp, sp}};
    res.min = point{50, 50};
    res.nominal_size = {150, 150};
    return res;
  }
  
  point layout(point sz) {
    point pos = {0, 0};
    float row_h = 0;
    
    /* 
    widget_size_info row_sz_info;
    
    for (auto& c : children_vec) {
      auto i = c.size_info();
      row_sz_info.min[0] += i.min[0];
      row_sz_info.min[1] = std::max(row_sz_info.min[1], i.min[1]);
    } */ 
    
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