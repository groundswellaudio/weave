#pragma once

#include <unordered_map>
#include <vector>
#include "events/mouse_events.hpp"
#include <functional>

struct painter;

struct layout_tag {};

struct view_id {
  bool operator==(const view_id&) const = default;
  unsigned value = 0;
};

struct widget_id {
  
  constexpr widget_id() = default;
  constexpr widget_id(view_id id) : base{id} {}
  
  bool operator==(const widget_id& o) const = default; 
  view_id base;
  unsigned offset = 0;
};

template <>
class std::hash<widget_id> {
  public : 
  constexpr std::size_t operator()(const widget_id& x) const { return (std::size_t(x.base.value << 32) | x.offset); }
};

template <class T>
struct event_context {
    
  // Used by implementation only.
  template <class Lens>
  void emplace(void* s, Lens l) {
    state_ptr = s;
    write_fn = [l] (void* s, const T& val) { l.write(val, s); };
    read_fn = [l] (void* s) { return l.read(s); };
  }
  
  void write(T v) {
    write_fn(state_ptr, v);
  }
  
  T read() const {
    return read_fn(state_ptr);
  }
  
  void* state_ptr;
  std::function<void(void*, const T&)> write_fn;
  std::function<T(void*)> read_fn;
};

struct widget_tree 
{
  struct children_view;
  
  struct widget
  {
    private : 
  
    struct model 
    {
      virtual void paint(painter& p, vec2f size, void* state) = 0;
      virtual vec2f layout(children_view v) = 0;
      virtual void on(input_event e, vec2f this_size, void* ctx_ptr) = 0;
      virtual ~model() {};
    };
  
    template <class T>
    struct model_impl_base : model {
      T obj;
    };
  
    template <class T, class Lens>
    struct model_impl final : model_impl_base<T> {
    
      void on(input_event e, vec2f this_size, void* ctx_ptr) override 
      {
        if constexpr (^Lens != ^void)
        {
          event_context<typename T::value_type> evctx {};
          evctx.template emplace<Lens>(ctx_ptr, Lens{});
          this->obj.on(e, this_size, evctx);
        }
        else 
        {
          this->obj.on(e);
        }
      }
    
      void paint(painter& p, vec2f size, void* state) override {
        //paint_context pc {Lens::read(ctx)};
        if constexpr (^Lens != ^void)
          this->obj.paint(p, Lens{}.read(state), size);
        else
          this->obj.paint(p, size);
      }
    
      vec2f layout(children_view v) override {
        if constexpr (is_base_of(^layout_tag, ^T))
          return this->obj.layout(v);
        return {0, 0};
      }
      
      ~model_impl() override {}
    };
  
    using Self = widget;
  
    public : 
    
    widget(widget_id this_id, widget_id parent_id)
    : this_id{this_id}, parent_id_v{parent_id}
    {}
    
    template <class T, class Lens, class... Ts>
    void emplace(Ts... ts) {
      model_ptr.reset( new model_impl<T, Lens> {ts...} );
    }
  
    auto layout(children_view v) {
      size_v = model_ptr->layout(v);
      return size_v;
    }
  
    template <class T>
    T& get_as() {
      return static_cast<model_impl_base<T>*>(model_ptr.get())->obj;
    }
    
    void paint(painter& p, void* state) {
      model_ptr->paint(p, size(), state);
    }
  
    void on(input_event e, void* state) {
      model_ptr->on(e, size(), state);
    }
    
    void set_size(vec2f sz) {
      size_v = sz;
    }
    
    vec2f size() const { return size_v; }
    vec2f pos() const { return pos_v; }
    
    void set_pos(float x, float y) {
      pos_v = vec2f{x, y};
    }
    
    auto parent_id() const { return parent_id_v; }
    auto id() const { return this_id; }
    
    void add_child(widget_id id) {
      children_v.push_back(id);
    }
    
    auto& children() const { return children_v; }
    
    private : 
    
    vec2f pos_v, size_v;
    widget_id parent_id_v, this_id;
    std::vector<widget_id> children_v;
    std::unique_ptr<model> model_ptr;
  };
  
  /* 
  struct node_and_children {
    widget node;
    std::vector<widget_id> children;
  };  */ 
  
  struct children_view {
    
    struct iterator 
    {
      auto& operator++() { ++it; return *this; }
      bool operator==(iterator o) const { return it == o.it; }
      auto& operator*() const { return self.get(*it); }
      
      std::vector<widget_id>::const_iterator it; 
      widget_tree& self;
    };
    
    auto begin() const {
      return iterator{child_vec.begin(), self};
    }
    
    auto end() const {
      return iterator{child_vec.end(), self};
    }
  
    const std::vector<widget_id>& child_vec;
    widget_tree& self;
  };
  
  auto children(widget_id id) {
    return children_view{get(id).children(), *this};
  }
  
  auto children(widget& w) {
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
  
  template <class T, class Lens = void>
  widget* create_widget(widget_id id, widget_id parent, vec2f size) {
    auto [it, success] = widget_map.try_emplace(id, widget{id, parent});
    it->second.emplace<T, Lens>();
    it->second.set_size(size);
    it->second.set_pos(0, 0);
    //get(parent).add_child(id);
    return &it->second;
  }
  
  widget* root() { return &widget_map.find(widget_id{view_id{0}})->second; }
  
  void layout() {
    root()->layout( children(widget_id{view_id{0}}) );
  }
  
  std::unordered_map<widget_id, widget> widget_map;
};

using widget = widget_tree::widget;

struct view {
  auto& operator=(const view&) { return *this; }
  widget_id view_id = {};
};

struct composed_view : view {};

struct widget_tree_builder 
{
  widget_tree& tree;
  widget_id current_id = {};
  
  template <class V>
  widget* construct_view(V view)
  {
    auto next_current = view.construct(*this);
    widget_tree_builder next {tree, next_current->id()};
    
    if constexpr ( std::meta::is_base_of(^composed_view, ^V) ) 
      view.traverse( [&next, next_current] (auto& elem) {
        auto child = next.construct_view(elem);
        next_current->add_child(child->id());
      });
    
    return next_current;
  }
  
  template <class T, class Lens = void>
  widget_tree::widget* create_widget(widget_id id, vec2f size) {
    return tree.create_widget<T, Lens>(id, current_id, size);
  }
};

struct view_archetype : view {
  
  widget_tree::widget* construct(widget_tree_builder& b);
    // { return b.tree.construct_widget(id); }
};

/* 
template <class T>
struct ui_container;

template <field_decl F, class T>
struct mutator_t 
{
  T object;
};

template <field_decl F, class T>
auto mutator(T object)
{
  return mutator_t<F, T>{object};
}

template <field_decl F>
struct mutator_path_element {};

template <field_decl F, class T>
struct ui_container<mutator_t<F, T>>
{
  template <class Self, class Fn>
  auto traverse(Self&& s, Fn fn) {
    return fn(s.object, mutator_path_element<F>{});
  }
};

// ui_tree_path

template <class... Path>
struct ui_tree_path;

template <>
struct ui_tree_path<> {};

template <class TopNode>
struct ui_tree_path<TopNode> : ui_tree_path<>
{
  using base = ui_tree_path<>;
  
  auto& operator()() {
    return *top_node;
  }
  
  TopNode* top_node;
};

template <class LastIndex, class... Path>
struct ui_tree_path<LastIndex, Path...> : ui_tree_path<Path...>
{
  using base = ui_tree_path<Path...>;
  
  auto& operator()() {
    auto& base = (ui_tree_path<Path...>&)(*this);
    return base().layout()[index];
  }
  
  auto pop_back() const {
    return base{*this};
  }
  
  [[no_unique_address]] LastIndex index;
};

template <field_decl F, class... Path>
struct ui_tree_path<mutator_path_element<F>, Path...> : ui_tree_path<Path...>
{
  using base = ui_tree_path<Path...>;
  
};

template <class Index, class... Path>
auto push_back(ui_tree_path<Path...> path, Index index)
{
  return ui_tree_path<Index, Path...>{path, index};
}
  
template <class... Path>
struct ui_tree_partial_path {};

template <>
struct ui_tree_partial_path<> {};

template <class Head, class... Tail>
struct ui_tree_partial_path<Head, Tail...> : ui_tree_partial_path<Tail...> {};

template <class Index, class... Path>
auto push_back(ui_tree_partial_path<Path...> path, Index index)
{
  return ui_tree_partial_path<Index, Path...>{};
}

template <class... Ls, class... Rs>
auto push_back(ui_tree_path<Ls...> lhs, ui_tree_partial_path<Rs...> rhs)
{
  return ui_tree_path<Rs..., Ls...>{lhs};
}

template <class Object, class Fn, class Path>
bool traverse(Object&& obj, Fn fn, Path path)
{
  using ObjT = %remove_reference(^Object);
  
  if constexpr ( requires { ui_container<ObjT>{}; } )
  {
    auto impl = [fn, path] (auto& obj, auto path_element) {
      return traverse(obj, fn, push_back(path, path_element));
    };
    return ui_container<ObjT>{}.traverse(obj, impl);
  }
  else
  {
    return fn(obj, path);
  }
}

template <class T>
struct state_read { using result = T; };

template <class T>
struct state_write { using result = void; T value; };

namespace impl 
{
  namespace is_direct_receiver_ns // FIXME : workaround 
  {
    template <class Signal, class Path>
    concept is_direct_receiver = requires (Signal sig, Path p) {
      p().receive(sig, (typename Path::base&) p);
    };
  }
  
  struct none { int none; };
  
  template <class T>
  using invoke_result = decltype( std::declval<T>()() );
  
  consteval class_decl parent_node(class_decl path) {
    auto lr = lookup(path, operator_kind::call);
    return (class_decl) remove_reference(return_type( *begin(lr) ));
  }
  
  consteval expr make_state_expr(expr path)
  {
    field_decl fd = ^none::none;
    auto path_ty = (class_template_type) remove_reference(type_of(path));
    
    while(true)
    {
      auto arg_ty = (type)arguments(path_ty)[0];
      
      if (is_instance_of(arg_ty, ^mutator_path_element))
      {
        auto arg = arguments((class_template_type) arg_ty)[0];
        fd = (field_decl) arguments((class_template_type) arg_ty)[0];
      }
      
      auto p = parent_node(path_ty);
      if (p == parent(fd))
      {
        path = make_static_cast_expr(path_ty, path);
        return make_field_expr(make_call_expr(path, expr_list{}), fd);
      }
      
      path_ty = (class_template_type) (type) (class_decl) bases(path_ty)[0];
    }
  }
  
  consteval type find_receiver(type signal, class_decl path) 
  {
    auto is_direct_receiver = (concept_decl) *begin(children(^is_direct_receiver_ns));
    while(true)
    {
      if ( satisfy(template_arguments{signal, (type)path}, is_direct_receiver) )
        return path;
      auto bs = bases(path);
      if (size(bs) == 0)
      {
        std::meta::ostream os;
        os << "error in emit : found no receiver in ui tree for signal " << signal;
        error(os);
      }
      path = (class_decl) bs[0];
    }
  }
}

template <class Signal, class... Path>
decltype(auto) emit(ui_tree_path<Path...> path, Signal signal)
{
  if constexpr (is_instance_of(^Signal, ^state_read))
  {
    return typename Signal::result( %impl::make_state_expr(^(path)) );
  }
  else if constexpr (is_instance_of(^Signal, ^state_write))
  {
    (%impl::make_state_expr(^(path)) = signal.value);
  }
  else
  {
    using Receiver = %impl::find_receiver(^Signal, ^ui_tree_path<Path...>);
    ((Receiver&)path)().receive(signal, (typename Receiver::base)path);
  }
} */ 