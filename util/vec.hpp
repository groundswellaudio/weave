
#include "gen-operators.hpp"

using namespace std::meta;

consteval void gen_operators(class_builder& b) {
  b << ^^ struct T {
    template <operator_kind Op>
      requires (std::meta::is_compound_assign(Op))
    constexpr auto& apply_op(const vec<T, N>& o)
    {
      for (int k = 0; k < N; ++k)
        ([: make_operator_expr(Op, ^^((*this)[k]), ^^(o[k])) :]);
      return *this;
    }
    
    template <operator_kind Op>
      requires (not std::meta::is_compound_assign(Op))
    constexpr auto apply_op(const vec<T, N>& o) const 
    {
      auto Res {*this};
      ([: make_operator_expr(std::meta::compound_equivalent(Op), ^^(Res), ^^(o)) :]);
      return Res;
    }
    
    constexpr bool operator==(const vec<T, N>& o) const {
      for (int k = 0; k < N; ++k)
        if ((*this)[k] != o[k])
          return false;
      return true;
    }
    
    constexpr bool operator!=(const vec<T, N>& o) const {
      return !(*this == o);
    }
    
    [: declare_arithmetic(^^const T&) :];
  };
}

template <class T, unsigned N>
struct vec 
{
  T data[N];
  
  decltype(auto) operator[](this auto&& self, int index) {
    return (self.data[index]);
  }
  
  [: gen_operators() :];
};

template <class T>
struct vec<T, 2> {
  
  T x, y;
  
  decltype(auto) operator[](this auto&& self, int index) {
    return (index ? self.y : self.x);
  }
  
  [: gen_operators() :];
};

consteval void glm_aliases(namespace_builder& b) {
  const char* tsuffix[] { "u", "i", "f", "d", "b" }; 
  type_list bases { ^^unsigned, ^^int, ^^float, ^^double, ^^bool };
  auto vecid = identifier{"vec"};
  for ( int k = 0; k < size(bases); ++k)
  {
    for (int dim = 2; dim < 5; ++dim)
    {
      auto type = type_of(substitute(^^vec, {bases[k], value_template_argument(dim)}));
      auto name = cat(cat(vecid, dim), tsuffix[k]);
      b << ^^ [type, name] namespace {
        using name[:name:] = [: type :];
      };
    }
  }
}

[: glm_aliases() :];