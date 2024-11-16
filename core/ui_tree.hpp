#pragma once

#include <unordered_map>
#include <vector>
#include <functional>
#include <iostream>
#include <cassert>
#include <span>

#include "events/mouse_events.hpp"
#include "lens.hpp"
#include "../util/tuple.hpp"

struct widget_tree;
struct painter;
struct widget_builder;
struct widget_updater;

struct widget_id {
  
  friend widget_builder; 
  
  private : 
  
  std::size_t value;
  
  constexpr widget_id(std::size_t v) : value{v} {}
  constexpr widget_id() = default;
  
  public : 
  
  bool operator==(const widget_id& o) const = default; 
  
  static constexpr widget_id root() { return {0}; }
  
  auto raw() const { return value; }
};

consteval std::meta::expr to_str(std::meta::type t) {
  std::meta::ostream os;
  os << t;
  return make_literal_expr(os);
}

template <class T>
auto& operator<<(std::ostream& os, const vec<T, 2>& v) {
  os << "{" << v.x << " " << v.y << "}";
  return os;
}

auto& operator<<(std::ostream& os, widget_id id) {
  os << id.raw();
  return os;
}

template <>
class std::hash<widget_id> {
  public : 
  constexpr std::size_t operator()(const widget_id& x) const { 
    return x.raw();
  }
};

struct widget_properties {
  vec2f pos, sz;
  widget_id this_id_v, parent_id_v;
  bool child_event_listener = false;
};

struct widget_ctor_args {
  widget_id id; 
  vec2f size;
};

template <class T, class EvCtx>
concept is_child_event_listener = requires (T obj, input_event e, widget_id i, 
                                            EvCtx ec) 
{
  obj.on_child_event(e, ec, i);
};
  
struct widget_tree 
{
  struct children_view;
  struct widget; 
  
  struct event_context_data {
    
    void* application_state() const { return state_ptr; }
    auto this_size() { return elem->size(); }
    
    widget_tree& tree;
    void* state_ptr;
    widget* elem = nullptr;
  };

  template <class T>
  struct event_context : event_context_data {
    
    template <class Lens>
    void emplace(Lens l) {
      write_fn = [l] (void* s, const T& val) { l(*static_cast<typename Lens::input*>(s)) = val; };
      read_fn = [l] (void* s) { return l(*static_cast<typename Lens::input*>(s)); };
    }
  
    void write(T v) {
      write_fn(state_ptr, v);
    }
  
    T read() const {
      return read_fn(state_ptr);
    }
    
    std::function<void(void*, const T&)> write_fn;
    std::function<T(void*)> read_fn;
  };
  
  template <>
  struct event_context<void> : event_context_data {};
  
  using widget_children = std::span<widget*>;
  
  struct widget
  {
    private : 
    
    template <class T>
    using ptr = T*;
    
    struct model 
    {
      virtual void debug_dump() = 0;
      virtual void paint(painter& p, vec2f size, void* state) = 0;
      virtual vec2f layout(vec2f size) = 0;
      virtual void on(input_event e, event_context_data ec) = 0;
      virtual void on_child_event(input_event e, event_context_data ec, widget_id child) = 0;
      virtual widget* find_child_at(vec2f pos) = 0;
      virtual widget_children children() = 0;
      
      virtual ~model() {};
    };
    
    template <class T>
    struct model_impl_base : model 
    {
      model_impl_base(auto&&... args) : obj{args...} {}
      
      void debug_dump() final { std::cerr << %to_str(^T); }
      
      widget* find_child_at(vec2f pos) final {
        if constexpr ( requires {obj.find_child_at(pos);} )
          return obj.find_child_at(pos);
        return nullptr;
      }
      
      vec2f layout(vec2f size) final {
        if constexpr ( requires {obj.layout();} )
          return this->obj.layout();
        return size;
      }
      
      widget_children children() final {
        if constexpr ( requires {obj.children();} )
          return obj.children();
        else
          return {};
      }
      
      T obj;
    };
  
    template <class T, class Lens>
    struct model_impl final : model_impl_base<T> {
      
      [[no_unique_address]] Lens lens;
      
      model_impl(Lens L, auto&&... args) : lens{L}, model_impl_base<T>{args...} {}
      
      using EvCtx = event_context<typename T::value_type>;
      
      void on(input_event e, event_context_data ec) override 
      {
        EvCtx evctx {ec};
        if constexpr (^Lens != ^empty_lens)
          evctx.emplace(lens);
        this->obj.on(e, evctx);
      }
      
      void on_child_event(input_event e, event_context_data ec, widget_id child) override 
      {
        if constexpr (::is_child_event_listener<T, EvCtx>)
        {
          EvCtx evctx {ec};
          if constexpr (^Lens != ^empty_lens)
            evctx.emplace(lens);
          this->obj.on_child_event(e, evctx, child);
        }
      }
    
      void paint(painter& p, vec2f size, void* state) override {
        if constexpr (^Lens != ^empty_lens) {
          auto sv = lens(*static_cast<typename Lens::input*>(state));
          this->obj.paint(p, sv, size);
        }
        else
          this->obj.paint(p, size);
      }
      
      ~model_impl() override {}
    };
  
    using Self = widget;
  
    public : 
    
    widget(widget_id this_id, widget_id parent_id)
    : this_id{this_id}, parent_id_v{parent_id}
    {}
    
    widget* find_child_at(vec2f pos) {
      return model_ptr->find_child_at(pos);
    }
    
    template <class T, class Lens, class... Ts>
    void emplace(Lens lens, Ts... ts) {
      auto ptr = new model_impl<T, Lens> {lens, ts...};
      child_event_listener = ::is_child_event_listener<T, event_context<typename T::value_type>>;
      model_ptr.reset( new model_impl<T, Lens> {lens, ts...} );
    }
    
    auto layout() {
      size_v = model_ptr->layout(size());
      return size_v;
    }
  
    template <class T>
    T& as() {
      return static_cast<model_impl_base<T>*>(model_ptr.get())->obj;
    }
    
    void paint(painter& p, void* state) {
      model_ptr->paint(p, size(), state);
    }
  
    void on(input_event e, event_context_data ec) {
      ec.elem = this;
      model_ptr->on(e, ec);
    }
    
    void on_child_event(input_event e, event_context_data ec, widget_id child) {
      ec.elem = this;
      model_ptr->on_child_event(e, ec, child);
    }
    
    void set_size(vec2f sz) {
      size_v = sz;
    }
    
    vec2f size() const { return size_v; }
    vec2f position() const { return pos_v; }
    
    void set_position(float x, float y) {
      pos_v = vec2f{x, y};
    }
    
    bool contains(vec2f pos) {
      auto p = position();
      auto sz = size();
      return p.x <= pos.x && p.y <= pos.y && p.x + sz.x >= pos.x && p.y + sz.y >= pos.y;
    }
    
    void set_parent_id(widget_id id) { parent_id_v = id; }
    
    auto parent_id() const { return parent_id_v; }
    auto id() const { return this_id; }
    
    vec2f layout(widget_tree& tree) {
      return model_ptr->layout(size());
    }
    
    bool is_child_event_listener() const { return child_event_listener; }
    
    widget_children children() const { return model_ptr->children(); }
    
    /* 
    void debug_dump(widget_tree& tree, int indentation) {
      std::cerr << std::endl;
      for (int k = 0; k < indentation; ++k)
        std::cerr << "\t";
      std::cerr << "pos " << pos_v  << " size " << size_v << " ";
      model_ptr->debug_dump();
      for (auto& c : tree.children(*this))
        c.debug_dump(tree, indentation + 1);
    } */ 
    
    private : 
    
    vec2f pos_v, size_v;
    widget_id parent_id_v, this_id;
    bool child_event_listener;
    std::unique_ptr<model> model_ptr;
  };
  
  struct children_view {
    
    struct iterator 
    {
      iterator& operator++() { ++it; return *this; }
      bool operator==(iterator o) const { return it == o.it; }
      widget& operator*() const { return **it; }
      
      std::span<widget*>::iterator it; 
      widget_tree& self;
    };
    
    auto& operator[](int idx) { return *child_vec[idx]; }
    iterator begin() const { return iterator{child_vec.begin(), self}; }
    iterator end() const { return iterator{child_vec.end(), self}; }
    widget_tree& tree() const { return self; }
  
    std::span<widget*> child_vec;
    widget_tree& self;
  };
  
  /// Tree functions
  
  children_view children(widget_id id) {
    return children_view{get(id).children(), *this};
  }
  
  children_view children(widget& w) {
    return children(w.id());
  } 
  
  widget& get(widget_id id) {
    auto it = widget_map.find(id);
    return it->second;
  }
  
  const widget& get(widget_id id) const {
    auto it = widget_map.find(id);
    return it->second;
  }
  
  widget* find(widget_id id) {
    auto it = widget_map.find(id);
    if (it == widget_map.end())
      return nullptr;
    return &it->second;
  }
  
  widget_id parent_id(widget_id id) const { return get(id).parent_id(); }
  
  widget& parent(widget& w) { return get(w.parent_id()); }
  widget& root() { return widget_map.find(widget_id::root())->second; }
  
  template <class W, class Lens>
  widget* create_widget(widget_id parent_id, W&& widget_obj, Lens lens, widget_ctor_args args) {
    auto [it, success] = widget_map.try_emplace(args.id, widget{args.id, parent_id});
    it->second.emplace<W>(lens, widget_obj);
    it->second.set_size(args.size);
    it->second.set_position(0, 0);
    return &it->second;
  }
  
  /* 
  void debug_dump() {
    root().debug_dump(*this, 0);
  } */ 
  
  void destroy(widget_id id) {
    auto it = widget_map.find(id);
    assert( it != widget_map.end() && "widget not found for id" );
    auto parent = it->second.parent_id();
    widget_map.erase(it);
  }
  
  widget_builder builder();
  
  widget_updater updater(); 
  
  private :
   
  friend widget_builder;
  friend widget_updater;
  
  template <class T, class Lens, class... Args>
  widget* create_widget(Lens lens, widget_id id, widget_id parent, vec2f size, Args&&... args) {
    auto [it, success] = widget_map.try_emplace(id, widget{id, parent});
    it->second.emplace<T>(lens, args...);
    it->second.set_size(size);
    it->second.set_position(0, 0);
    return &it->second;
  }
  
  unsigned id_counter = 0;
  std::unordered_map<widget_id, widget> widget_map;
};

template <class T>
using event_context = widget_tree::event_context<T>;

using widget = widget_tree::widget;
using event_context_data = widget_tree::event_context_data;

struct widget_builder 
{
  friend widget_tree;
  
  unsigned& id_counter;
  widget_tree& tree;
  
  auto next_id() {
    return widget_id{id_counter++};
  }
  
  template <class W, class Lens>
  widget* make_widget(widget_id parent_id, W widget, Lens lens, widget_ctor_args args) {
    auto w = tree.create_widget(parent_id, std::move(widget), lens, args);
    w->set_parent_id(parent_id);
    return w;
  }
};

inline widget_builder widget_tree::builder() {
  return {id_counter, *this};
}

struct widget_updater 
{
  widget* parent_widget() const { return parent_v; }
  
  auto with_parent(widget* w) { widget_updater res{*this}; res.parent_v = w; return res; }
   
  widget_builder builder() { return {id_counter, tree}; }
  
  unsigned& id_counter;
  widget_tree& tree;
  widget* parent_v;
};

inline widget_updater widget_tree::updater() {
  return {id_counter, *this, &root()};
}

/* 
struct view_archetype : view {
  
  widget_tree::widget* construct(widget_tree_builder& b);
    // { return b.tree.construct_widget(id); }
}; */ 