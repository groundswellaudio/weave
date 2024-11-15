#pragma once

#include <unordered_map>
#include <vector>
#include <functional>
#include <iostream>
#include <cassert>

#include "events/mouse_events.hpp"
#include "view.hpp"
#include "lens.hpp"
#include "../util/tuple.hpp"

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
  os << id.base.value << "." << id.offset;
  return os;
}

struct painter; 

struct widget_tree_builder;
struct widget_tree_updater;

struct widget_tree 
{
  struct children_view;
  
  struct widget; 
  
  struct event_context_data {
    widget_tree& tree;
    void* state_ptr;
    widget* elem = nullptr;
  };
  
  template <class T>
  struct event_context {
    
    template <class Lens>
    void emplace(Lens l) {
      write_fn = [l] (void* s, const T& val) { l(*static_cast<typename Lens::input*>(s)) = val; };
      read_fn = [l] (void* s) { return l(*static_cast<typename Lens::input*>(s)); };
    }
  
    void write(T v) {
      write_fn(data.state_ptr, v);
    }
  
    T read() const {
      return read_fn(data.state_ptr);
    }
    
    auto children() {
      return data.tree.children(elem);
    }
    
    event_context_data data;
    std::function<void(void*, const T&)> write_fn;
    std::function<T(void*)> read_fn;
  };
  
  struct widget
  {
    private : 
    
    template <class T>
    using ptr = T*;
    
    struct model 
    {
      virtual void debug_dump() = 0;
      virtual void paint(painter& p, vec2f size, void* state) = 0;
      virtual vec2f layout(children_view v, vec2f size) = 0;
      virtual void on(input_event e, event_context_data ec) = 0;
      virtual void on_child_event(input_event e, event_context_data ec) = 0;
      virtual ~model() {};
    };
    
    template <class T>
    concept is_child_event_listener = requires (input_event e, widget_id i, 
                                                event_context<typename T::value_type> ec) 
    {
      obj.on_child_event(e, i, ec); 
    };
    
    template <class T>
    struct model_impl_base : model {
      model_impl_base(auto&&... args) : obj{args...} {}
      
      bool is_child_event_listener() const { 
        return requires (input_event e, widget_id i, event_context<typename T::value_type> ec) { 
          obj.on_child_event(e, i, ec); 
        };
      }
      
      T obj;
    };
  
    template <class T, class Lens>
    struct model_impl final : model_impl_base<T> {
      
      [[no_unique_address]] Lens lens;
      
      model_impl(Lens L, auto&&... args) : lens{L}, model_impl_base<T>{args...} {}
      
      void debug_dump() override { 
        std::cerr << %to_str(^T);
      }
      
      void on(input_event e, event_context_data ec) override 
      {
        if constexpr (^Lens != ^empty_lens)
        {
          event_context<typename T::value_type> evctx {ec};
          evctx.emplace(lens);
          this->obj.on(e, this_size, evctx);
        }
        else 
        {
          this->obj.on(e, ec.widget->size(), ec.state_ptr);
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
    
      vec2f layout(children_view v, vec2f size) override {
        if constexpr (is_base_of(^layout_tag, ^T))
          return this->obj.layout(v);
        return size;
      }
      
      ~model_impl() override {}
    };
  
    using Self = widget;
  
    public : 
    
    widget(widget_id this_id, widget_id parent_id)
    : this_id{this_id}, parent_id_v{parent_id}
    {}
    
    template <class T, class Lens, class... Ts>
    void emplace(Lens lens, Ts... ts) {
      auto ptr = new model_impl<T, Lens> {lens, ts...};
      child_event_listener = ptr->is_child_event_listener();
      model_ptr.reset( new model_impl<T, Lens> {lens, ts...} );
    }
  
    auto layout(children_view v) {
      size_v = model_ptr->layout(v, size());
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
      ec.widget = this;
      model_ptr->on(e, size(), ec);
    }
    
    void set_size(vec2f sz) {
      size_v = sz;
    }
    
    vec2f size() const { return size_v; }
    vec2f position() const { return pos_v; }
    
    void set_position(float x, float y) {
      pos_v = vec2f{x, y};
    }
    
    auto parent_id() const { return parent_id_v; }
    auto id() const { return this_id; }
    
    void add_child(widget_id id) {
      children_v.push_back(id);
    }
    
    void insert_child(int pos, widget_id id) {
      children_v.insert(children_v.begin() + pos, id);
    }
    
    auto& children() const { return children_v; }
    
    vec2f layout(widget_tree& tree) {
      return this->layout( children_view{children_v, tree} );
    }
    
    void debug_dump(widget_tree& tree, int indentation) {
      std::cerr << std::endl;
      for (int k = 0; k < indentation; ++k)
        std::cerr << "\t";
      std::cerr << "pos " << pos_v  << " size " << size_v << " ";
      model_ptr->debug_dump();
      for (auto& c : tree.children(*this))
        c.debug_dump(tree, indentation + 1);
    }
    
    void remove_child(widget_id id) {
      auto it = std::find(children_v.begin(), children_v.end(), id);
      assert( it != children_v.end() && "id not contained in children" );
      children_v.erase(it);
    }
    
    private : 
    
    vec2f pos_v, size_v;
    widget_id parent_id_v, this_id;
    std::vector<widget_id> children_v;
    std::unique_ptr<model> model_ptr;
  };
  
  struct children_view {
    
    struct iterator 
    {
      iterator& operator++() { ++it; return *this; }
      bool operator==(iterator o) const { return it == o.it; }
      widget& operator*() const { return self.get(*it); }
      
      std::vector<widget_id>::const_iterator it; 
      widget_tree& self;
    };
    
    iterator begin() const {
      return iterator{child_vec.begin(), self};
    }
    
    iterator end() const {
      return iterator{child_vec.end(), self};
    }
    
    widget_tree& tree() const { return self; }
  
    const std::vector<widget_id>& child_vec;
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
  
  widget* find(widget_id id) {
    auto it = widget_map.find(id);
    if (it == widget_map.end())
      return nullptr;
    return &it->second;
  }
  
  widget_id parent_id(widget_id id) const { return get(id).parent_id(); }
  
  widget& parent(widget& w) { return get(w.parent_id()); }
  
  widget* root() { return &widget_map.find(widget_id{view_id{0}})->second; }
  
  void layout() {
    root()->layout(*this);
  }
  
  void debug_dump() {
    root()->debug_dump(*this, 0);
  }
  
  void destroy(widget_id id) {
    auto it = widget_map.find(id);
    assert( it != widget_map.end() && "widget not found for id" );
    auto parent = it->second.parent_id();
    find(parent)->remove_child(id);
    widget_map.erase(it);
  }
  
  widget_tree_builder builder();
  
  widget_tree_updater updater(); 
  
  private :
   
  friend widget_tree_builder;
  friend widget_tree_updater;
  
  template <class T, class Lens, class... Args>
  widget* create_widget(Lens lens, widget_id id, widget_id parent, vec2f size, Args&&... args) {
    auto [it, success] = widget_map.try_emplace(id, widget{id, parent});
    it->second.emplace<T>(lens, args...);
    it->second.set_size(size);
    it->second.set_pos(0, 0);
    return &it->second;
  }
  
  unsigned id_counter = 0;
  std::unordered_map<widget_id, widget> widget_map;
};

using widget = widget_tree::widget;

struct widget_tree_builder 
{
  friend widget_tree;
  
  unsigned& id_counter;
  widget_tree& tree;
  widget* parent_v;
  int* child_index = nullptr;
  
  // Create a widget and returns a tree builder to build its children, if needed.
  template <class T, class Lens>
  widget_tree_builder create_widget(Lens lens, vec2f size, auto&&... args) 
  {
    auto res = tree.create_widget<T>(lens, widget_id{view_id{id_counter++}}, parent_v ? parent_v->id() : widget_id{view_id{0}}, size, args...);
    if (parent_v) {
      if (child_index) {
        parent_widget()->insert_child((*child_index)++, res->id());
      }
      else {
        parent_widget()->add_child(res->id());
      }
      assert( parent_widget()->id() != res->id() && "widget id invalids" );
    }
    return widget_tree_builder{id_counter, tree, res};
  }
  
  widget* parent_widget() const { return parent_v; }
};

inline widget_tree_builder widget_tree::builder() {
  return {id_counter, *this, nullptr};
}

struct widget_tree_updater 
{
  widget& consume_leaf() {
    if (parent_v)
      return tree.get(parent_v->children()[child_index++]);
    return tree.get(widget_id::root());
  }
  
  tuple<widget&, widget_tree_updater> consume_parent() {
    auto& r = consume_leaf();
    return {r, {id_counter, tree, &r, 0}};
  }
  
  template <class T, class Lens>
  widget_tree_builder create_widget(Lens lens, vec2f size, auto&&... args) {
    return builder().create_widget(lens, size, args...);
  }
  
  widget* parent_widget() const { return parent_v; }
  
  widget_tree_builder builder() { return {id_counter, tree, parent_v, &child_index}; }
  
  unsigned& id_counter;
  widget_tree& tree;
  widget* parent_v;
  int child_index;
};

inline widget_tree_updater widget_tree::updater() {
  return {id_counter, *this, nullptr, 0};
}

/* 
struct view_archetype : view {
  
  widget_tree::widget* construct(widget_tree_builder& b);
    // { return b.tree.construct_widget(id); }
}; */ 