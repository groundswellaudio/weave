//
//  keyboard.hpp
//  flux
//
//  Created by JB Vallon Hoarau on 07/09/2021.
//  Copyright © 2021 JB Vallon Hoarau. All rights reserved.
//

#pragma once

#include <SDL.h>

// the identifier for a (virtual) key
enum class keycode : unsigned char {
  // n0, n1...
  #define N(NUM) n##NUM = #NUM [0],
  #define K(name, CAPNAME) name,
  #define L(n, C) n = #n[0],
  #define S(name, CAPNAME, Z) name,
  
    #include "keys.def"
  
  #undef N
  #undef K
  #undef L
  #undef S
  invalid_key
};

enum class key_modifier : unsigned short {
  none = 0, 
  lshift = 1,
  rshift = 2,
  lctrl = 4,
  rctrl = 8,
  lalt = 16,
  ralt = 32,
  lgui = 64, 
  rgui = 128,
  num = 256,
  caps = 512,
  mode = 1024,
   
  ctrl = lctrl | rctrl,
  shift = rshift | lshift, 
  alt = ralt | lalt,
  gui = lgui | rgui
};

constexpr key_modifier operator|(key_modifier a, key_modifier b) {
  return (key_modifier)( (unsigned char) a | (unsigned char) b);
}

constexpr bool operator&(key_modifier a, key_modifier b) {
  return (unsigned char) a & (unsigned char) b;
}

struct keyboard_event {
  keycode key;
  bool is_press;
  
  bool is_key_down() const { return is_press; }
  bool is_key_up() const { return !is_press; }
};

inline bool is_number(keycode k){
  #define N(NUM) case keycode::n##NUM : return true;
  #define K(x, y)
  #define L(x, y)
  #define S(x, y, z)
  switch(k)
  {
    #include "keys.def"
    default : return false;
  }
  #undef N
  #undef K
  #undef L
  #undef S
}

inline char to_integer(keycode k){
  return static_cast<std::underlying_type_t<keycode>>(k) + '0';
}

inline char to_character(keycode k){
  return static_cast<std::underlying_type_t<keycode>>(k);
}

// backend stuff
namespace impl {

  inline keycode from_sdl_keycode(SDL_Keysym s){
    switch(s.sym)
    {
      #define N(NUM)  case SDLK_##NUM   : return keycode::n##NUM;
      #define K(N, C) case SDLK_##C     : return keycode::N;
      #define L(N, C) case SDLK_##N     : return keycode::N;
      #define S(N, C, Z) case SDLK_##C  : return keycode::N;
        #include "keys.def"
      #undef K
      #undef N
      #undef L
      #undef S
      default :
        return keycode::invalid_key;
    }
  }

  inline SDL_KeyCode to_sdl_keycode(keycode k)
  {
    switch(k)
    {
      #define N(NUM)  case keycode::n##NUM : return SDLK_##NUM;
      #define K(N, C) case keycode::N      : return SDLK_##C;
      #define L(N, C) case keycode::N      : return SDLK_##N;
      #define S(N, C, Z) case keycode::N   : return SDLK_##C;
        #include "keys.def"
      #undef K
      #undef N
      #undef L
      #undef S
      default :
        return SDLK_0;
    }
  }

} // impl

inline bool is_being_held(keycode k)
{
  auto* state = SDL_GetKeyboardState(nullptr);
  return state[SDL_GetScancodeFromKey(impl::to_sdl_keycode(k))];
}
