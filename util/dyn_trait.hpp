#pragma once

using namespace std::meta;

consteval type transpose_obj_type(type new_obj, type obj)
{
  if (is_const(remove_reference(obj)))
    new_obj = make_const(new_obj);
  if (is_lvalue_reference(obj))
    new_obj = make_lvalue_reference(new_obj);
  else if (is_rvalue_reference(obj))
    new_obj = make_rvalue_reference(new_obj);
  return new_obj;
}

consteval void append_types_of(type_list& tl, auto fl) {
  for (auto p : fl)
    push_back(tl, type_of(p));
}

consteval void expand_as_fields(class_builder& b, type_list tl, identifier id) {
  int k = 0;
  for (auto t : tl)
  {
    b << ^ [t, id, p = k++] struct {
      %typename(t) %name(cat(id, p)); 
    };
  }
}

consteval expr_list gen_vtable_for(type t, type trait) {
  expr_list res;
  for (auto m : methods((class_decl)trait)) 
  {
    type_list params_ty;
    append_types_of(params_ty, inner_parameters(m));
    auto e = ^[params_ty, t, m] ( +[] (void* self, %typename...(params_ty)... params) {
      auto& obj = *static_cast< %(t)* >(self);
      return obj.%(name_of(m))( (decltype(params)&&)(params)... );
    });
    
    push_back(res, e); 
  }
  return res;
}

consteval void gen_interface(class_builder& b, class_decl cd) 
{
  int k = 0;
  type_list vtable_fields; 
  
  for (auto m : methods(cd))
  {
    type_list params_ty;
    append_types_of(params_ty, inner_parameters(m));
    
    auto new_obj = transpose_obj_type(decl_of(b), object_type(m));

    b << ^[params_ty, new_obj, p = k++, m] struct T 
    {
      constexpr %typename(return_type(m)) %name(name_of(m)) (%typename...(params_ty)... params) {
        return this->vtable_ptr->%(cat("m", p))(this->data, params...);
      }
    };
    
    type_list vtable_fn_args {^void*};
    append_types_of(vtable_fn_args, inner_parameters(m));
    type fty = make_pointer(make_function(return_type(m), vtable_fn_args));
    push_back(vtable_fields, fty);
  }
  
  b << ^ [vtable_fields] struct T {
    struct vtable_data {
       %expand_as_fields(vtable_fields, "m");
    };
    
    template <class P>
    static constexpr vtable_data vtable_for {
      %...gen_vtable_for(^P, ^typename T::trait)...
    };
    
    const vtable_data* vtable_ptr; 
  };
}

template <class T>
struct defer_lookup {};

template <class IFace>
struct dyn_trait : defer_lookup<IFace>
{
  using trait = IFace;
  
  void* data;
  
  %gen_interface(^IFace);
  
  template <class T>
  constexpr dyn_trait(T* obj) {
    set(obj);
  }
  
  template <class T>
  constexpr void set(T* obj) {
    data = obj;
    this->vtable_ptr = &this->template vtable_for<T>;
  }
};