#pragma once

#include <concepts>
#include "vec.hpp"
#include "util/util.hpp"
#include "ui_tree.hpp"
#include <iostream>
#include <span>

/* 
struct vstack_node {
  void layout()
};

template <class... Ts>
struct vstack 
{
  template <class Ctx>
  void construct(Ctx ctx) 
  {
    auto new_ctx = ctx.construct_stateless<vstack_node>();
    apply( [&new_ctx] (auto&&... c) 
    {
      c.construct(ctx);
    }, children);
  }
  
  tuple<Ts...> children;
}; */ 
