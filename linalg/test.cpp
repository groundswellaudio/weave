// RUN: %clang++ -x c++ -std=c++20 -stdlib=libc++ -Xclang -verify -fsyntax-only %s
// expected-no-diagnostics

#include "vec.hpp"
#include "matrix.hpp"

using namespace std::meta;

using namespace gl_types;

consteval void test()
{
  ensure( (type)^vec2f == ^vec<float, 2> );
  ensure( (type)^vec3f == ^vec<float, 3> );
  ensure( (type)^vec4i == ^vec<int, 4> );
  
  vec2i x {1, 2};
  vec2i y {3, 4};
  
  ensure( x + y == vec2i{4, 6} );
  ensure( x - y == vec2i{-2, -2} );
  ensure( x * y == vec2i{3, 8} );
  ensure( x / y == vec2i{0, 0} );
  
  x += y;
  ensure( x == vec2i{4, 6} );
  x -= y;
  ensure( x == vec2i{1, 2} );
  x *= y;
  ensure( x == vec2i{3, 8} );
  x /= y;
  ensure( x == vec2i{1, 2} );
}

consteval void test_matrix()
{
  using mat2i = matrix<int, 2>;
  
  mat2i m {
    1, 2,
    4, 5
  };
  
  m *= 2;
  ensure( m == mat2i{ 2, 4, 8, 10 } );
  
  m -= 1;
  ensure( m == mat2i{1, 3, 7, 9} );
}

int main()
{
  test();
  test_matrix();
}