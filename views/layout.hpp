#pragma once

#include "views_core.hpp"
#include "scrollable.hpp"
#include "modifiers.hpp"
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
    return WEAVE_FWD(self);
  }
  
  auto&& align(this auto&& self, float ratio) {
    self.info.align_ratio = ratio;
    return WEAVE_FWD(self);
  }
  
  auto&& align_center(this auto&& self) {
    self.info.align_ratio = 0.5;
    return WEAVE_FWD(self);
  }
  
  auto&& background_color(this auto&& self, rgba_u8 col) {
    self.info.background_col = col;
    return WEAVE_FWD(self);
  }
  
  auto&& fill_cross_axis(this auto&& self) {
    self.info.fill_cross_axis = true;
    return WEAVE_FWD(self);
  }
  
  tuple<Ts...> children;
  stack_data info;
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
      pos[!axis] = data.margin[!axis] + data.align_ratio * 
                                (this_size[!axis] - c.size()[!axis] - 2 * data.margin[!axis]);
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
    float occupied_axis = 2 * data.margin[axis];
    for (auto& c : children)
      occupied_axis += c.size()[axis];
    occupied_axis += data.interspace * (children.size() - 1);
    
    float remaining_space = this_size[axis] - occupied_axis;
    
    for (int k = 0; k < 3; ++k)
    {
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
            sz[1 - axis] = axis ? sz[axis] / *sz_infos[i].aspect_ratio : sz[axis] * *sz_infos[i].aspect_ratio;
          if (sz[1 - axis] > sz_infos[i].max[1 - axis]
              || sz[1 - axis] > this_size[1 - axis] - 2 * data.margin[1 - axis])
          {
            sz[1 - axis] = std::min(sz_infos[i].max[1 - axis], this_size[1 - axis] - 2 * data.margin[1 - axis]);
            sz[axis] = axis ? sz.x * *sz_infos[i].aspect_ratio : sz.y / *sz_infos[i].aspect_ratio;
          }
          else if (sz[1 - axis] < sz_infos[i].min[1 - axis]) {
            sz[1 - axis] = sz_infos[i].min[1 - axis];
            sz[axis] = axis ? sz.x * *sz_infos[i].aspect_ratio : sz.y / *sz_infos[i].aspect_ratio;
          }
          children[i].set_size(sz);
          remaining_space += old_sz_axis - sz[axis]; 
        }
        else if (!sz_infos[i].aspect_ratio)
          unconstrained.push_back(children[i].borrow());
      }
      
      if (std::abs(remaining_space) < 1e-3 || !unconstrained.size())
        return;
      
      float sum_unconstrained_flex = 0;
      
      for (auto u : unconstrained)
        sum_unconstrained_flex += u.size_info().flex_factor[axis];
        
      // None of the remaining widget are flexible, nothing to do
      if (sum_unconstrained_flex == 0)
        return;
      
      for (auto u : unconstrained) {
        auto sz = u.size();
        auto szi = u.size_info();
        sz[axis] += remaining_space * szi.flex_factor[axis] / sum_unconstrained_flex;
        sz[axis] = std::min(szi.max[axis], sz[axis]);
        u.set_size(sz);
      }
      
      remaining_space = 0;
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
  rebuild_result rebuild_stack(V& New, auto& Old, widget_ref wb, const build_context& ctx, S& state) {
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
    
    // FIXME : we're not calling .destroy on the views! (implementing this is actually a bit tricky)
    
    // Remove every elements at the (sorted) indexes in to_erase
    if (seq_updater.to_erase.size())
    {
      auto it = std::remove_if( w.children_vec.begin(), w.children_vec.end(), 
                                [ida = 0, idb = 0, &seq_updater] (ignore) mutable {
                                  if (idb == seq_updater.to_erase.size())
                                    return false;
                                  if (ida++ == seq_updater.to_erase[idb]) {
                                    idb++;
                                    return true;
                                  }
                                  return false;
                                } );
      w.children_vec.erase(it, w.children_vec.end());
    }
    
    // If some underlying items were deleted or added, reset the scrollbar position
    if (seq_updater.mutated)
      w.reset_scrollbar();
      
    if (seq_updater.mutated || (res & rebuild_result::size_change)) {
      w.do_layout(w.size());
      return {};
    }
    
    return res;
  }
  
} // impl

namespace widgets {

template <int Axis>
struct stack : widget_base, scrollable_base
{
  stack_data data;
  std::vector<widget_box> children_vec;
  
  float scroll_offset = 0, total_height = 0;
  // if scrollable
  optional<float> min_scroll_axis;
  
  stack(stack_data d) : data{d} {}
  
  rectangle scroll_zone() const {
    return {{0, 0}, size()};
  }
  
  float scroll_size() const {
    return total_height; 
  }
  
  void displace_scroll(float delta) {
    for (auto& c : children_vec) {
      auto p = c.position();
      p[Axis] -= delta;
      c.set_position(p);
    }
    scroll_offset += delta;
  }
  
  void on(mouse_event e, event_context& ec) {
    if (min_scroll_axis)
      scrollable_base::on(e, ec);
  }
  
  void on_child_event(mouse_event e, widget_ref r, event_context& ec) {
    if (min_scroll_axis)
      scrollable_base::on_child_event(e, r, ec);
  }
  
  void paint(painter& p) {
    p.fill_style(data.background_col);
    p.fill(rectangle(size()));
    if (min_scroll_axis)
      scrollable_base::paint(p);
    // p.stroke_style(colors::green);
    //  p.stroke(rectangle(size()));
  }
  
  auto size_info() const {
    widget_size_info res;
    
    res.flex_factor = point{0, 0};
    
    if (!children_vec.size()) {
      return res;
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
    res.min += data.margin * 2.f;
    res.max += data.margin * 2.f;
    res.nominal[Axis] += data.margin[Axis] * 2.f + (children_vec.size() - 1) * data.interspace;
    res.nominal[1 - Axis] += data.margin[1 - Axis] * 2.f;
    // Average the flex factor cross axis
    res.flex_factor[1 - Axis] /= children_vec.size();
    
    if (min_scroll_axis) {
      res.min[Axis] = *min_scroll_axis;
      res.flex_factor[Axis] = 1;
      res.min[1 - Axis] += scrollable_base::bar_width;
      res.nominal[1 - Axis] += scrollable_base::bar_width;
      res.max[1 - Axis] += scrollable_base::bar_width;
    }
    
    return res;
  }
  
  bool traverse_children(auto&& fn) {
    return std::all_of(children_vec.begin(), children_vec.end(), fn);
  }
  
  void layout(point sz) {
    if (min_scroll_axis)
      sz[1 - Axis] -= scrollable_base::bar_width;
    impl::stack_layout(children_vec, data, Axis, sz);
    if (children_vec.size()) {
      auto& w = children_vec.back();
      total_height = w.position()[Axis] + w.size()[Axis];
    }
    if (min_scroll_axis)
    {
      for (auto& c : children_vec) {
        auto p = c.position();
        p[Axis] -= scroll_offset;
        c.set_position(p);
      }
    }
  }
};

using hstack = stack<0>;
using vstack = stack<1>;

struct flow : widget_base, scrollable_base {
  
  rectangle scroll_zone() const {
    return {{0, 0}, size()};
  }
  
  float scroll_size() const {
    return total_height; 
  }
  
  void displace_scroll(float delta) {
    for (auto& c : children_vec)
      c.set_position(c.position() - point{0, delta});
    scroll_offset += delta;
  }
  
  auto size_info() const {
    widget_size_info res;
    res.min = nominal_size / 2; 
    res.nominal = nominal_size;
    res.flex_factor = flex_factor;
    return res;
  }
  
  void layout(point sz) 
  {
    if (!children_vec.size())
      return;
    
    auto row_begin = children_vec.begin();
    auto row_end = children_vec.begin();
    float p = margin.x;
    float y = margin.y;
    
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
        stack_prop.margin.y = 0;
        impl::stack_layout(std::span{row_begin, row_end}, stack_prop, 0, point{sz.x, row_height});
        for (auto ic = row_begin; ic != row_end; ++ic) 
          ic->set_position(ic->position() + point{0, y - scroll_offset});
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
          total_height = y + row_height + margin.y;
          return;
        }
      }
    }
  }
  
  auto traverse_children(auto&& fn) {
    return std::all_of(children_vec.begin(), children_vec.end(), fn);
  }
  
  void paint(painter& p) {
    p.fill_style(background_color);
    p.fill(rounded_rectangle(size(), rounded_radius));
    scrollable_base::paint(p);
  }
  
  void on(mouse_event e, event_context& ec) {
    scrollable_base::on(e, ec);
  }
  
  std::vector<widget_box> children_vec;
  point nominal_size = {300, 300};
  point interspace = {5, 5};
  point margin = {5, 5};
  point flex_factor = {1, 1};
  rgba_u8 background_color = colors::black;
  float rounded_radius = 0;
  float scroll_offset = 0;
  float total_height = 0;
};

} // widgets

namespace views 
{

template <class T, class... Ts>
struct stack_base : view<stack_base<T, Ts...>>, stack<Ts...> {
  
  optional<float> min_scroll_sz;
  
  template <class... Vs>
  constexpr stack_base(Vs&&... ts) : stack<Ts...>{ {(Vs&&)(ts)...} } {} 
  
  stack_base(stack_base&& o) = default;
  stack_base(const stack_base&) = default;
  
  template <class S>
  auto build(const build_context& ctx, S& state) 
  {
    auto Res = impl::build_stack<T>(*this, ctx, state, this->info);
    Res.min_scroll_axis = min_scroll_sz;
    return Res;
  }
  
  template <class S>
  rebuild_result rebuild(auto& Old, widget_ref wb, const build_context& ctx, S& state) {
    auto& wl = wb.as<T>();
    wl.min_scroll_axis = min_scroll_sz;
    return impl::rebuild_stack<T>(*this, Old, wb, ctx, state);
  }
  
  void destroy(widget_ref w, application_context& ctx) {
    auto& wb = w.as<T>();
    auto lift_destroy = [it = wb.children_vec.begin()] mutable {
      return (it++)->borrow();
    };
    tuple_for_each( [&] (auto& elem) {
      elem.seq_destroy(lift_destroy, ctx);
    }, this->children );
  }
  
  auto&& scrollable(this auto&& self, float min_scroll_size = 300) {
    self.min_scroll_sz = min_scroll_size;
    return self;
  }
};

template <class... Ts>
  requires (is_view_sequence<Ts> && ...)
struct vstack : stack_base<widgets::stack<1>, Ts...>, view_modifiers
{ 
  using widget_t = widgets::stack<1>;
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
struct hstack : stack_base<widgets::stack<0>, Ts...>, view_modifiers
{ 
  using widget_t = widgets::stack<0>;
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
    res.rounded_radius = rounded_radius;
    return res;
  }
  
  auto rebuild(flow<Ts...>& Old, widget_ref w, auto&& ctx, auto& state) {
    auto& f = w.as<widgets::flow>();
    f.rounded_radius = rounded_radius;
    return impl::rebuild_stack<widgets::flow>(*this, Old, w, ctx, state);
  }
  
  void destroy(widget_ref r, application_context& ctx) {
    auto& wb = r.as<widgets::flow>();
    auto lift_destroy = [it = wb.children_vec.begin()] mutable {
      return (it++)->borrow();
    };
    tuple_for_each( [&] (auto& elem) {
      elem.seq_destroy(lift_destroy, ctx);
    }, children );
  }
  
  auto& rounded(float val) {
    rounded_radius = val;
    return *this;
  }
  
  float width;
  tuple<Ts...> children;
  float rounded_radius = 0;
};

template <class... Ts>
flow(float, Ts... ts) -> flow<Ts...>;  

} // views

} // weave