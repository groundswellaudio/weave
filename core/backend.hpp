
#pragma once

#include <SDL.h>
#include <glad/glad.h>

#include "events/mouse_events.hpp"
#include "events/keyboard.hpp"
#include <iostream>

namespace weave::impl {

class sdl_backend
{
  bool mouse_is_dragging = false;
  int last_mouse_down = 0;
  bool has_resized = false;
  
  void init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
      fprintf(stderr, "failed to initialize SDL, aborting.");
      exit(1);
    }
  
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  
    // setting up a stencil buffer is necessary for nanovg to work
    SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 1 );
    
    SDL_StartTextInput();
  }
  
  public :
  
  bool want_quit = false;
  
  template <class Ctx>
  static int on_window_resize(void* data, SDL_Event* event)
  {
    if (event->type == SDL_WINDOWEVENT && event->window.event == SDL_WINDOWEVENT_RESIZED)
      ((Ctx*)data)->on_window_resize();
    return 1;
  }
  
  template <class Ctx>
  sdl_backend(Ctx* ctx)
  {
    init();
    SDL_AddEventWatch( &on_window_resize<Ctx>, ctx );
  }
  
  void load_opengl()
  {
    if (!gladLoadGLLoader((GLADloadproc) SDL_GL_GetProcAddress))
		{
			fprintf(stderr, "failed to initialize the OpenGL context.");
			exit(1);
		}
  }
  
  template <class Fn>
  void visit_event(Fn vis) 
  {
    SDL_Event e;
    if (not SDL_WaitEventTimeout(&e, 15))
      return;
    
    switch(e.type)
    {
      case SDL_MOUSEBUTTONDOWN :
      {
        auto time = e.button.timestamp;
        bool is_double_click = false;
        if (time - last_mouse_down < 250) {
          last_mouse_down = 0;
          is_double_click = true;
        } else {
          last_mouse_down = time;
        }
        mouse_is_dragging = true;
        auto mde = mouse_down{to_mouse_button(e.button.button), is_double_click};
        auto ev = mouse_event{pos(e.button.x, e.button.y), mde};
        vis(ev); 
        break;
      }
      
      case SDL_MOUSEBUTTONUP :
      {
        mouse_is_dragging = false;
        auto p = pos(e.button.x, e.button.y);
        vis( mouse_event{p, mouse_up{}} );
        break;
      }
      
      case SDL_MOUSEMOTION :
      {
        auto p = pos(e.motion.x, e.motion.y);
        auto delta = pos(e.motion.xrel, e.motion.yrel);
        vis( mouse_event{p, mouse_move{delta, mouse_is_dragging, to_mouse_button(e.button.button)}} );
        break;
      }
      
      case SDL_MOUSEWHEEL :
      {
        auto delta = pos(e.wheel.x, e.wheel.y);
        auto ev = mouse_event{pos(e.wheel.mouseX, e.wheel.mouseY), mouse_scroll{delta}};
        vis(ev);
        break;
      }
    
      case SDL_TEXTEDITING :
      case SDL_TEXTINPUT :
      {
        break;
      }
      
      case SDL_DROPFILE: 
      {
        std::string file = e.drop.file;
        SDL_free(e.drop.file);
        vis( mouse_event{vec2f{0, 0}, file_drop(std::move(file))} );
        break;
      }
      
      case SDL_DROPTEXT:
      case SDL_DROPBEGIN:
      case SDL_DROPCOMPLETE: 
        break;
      
      case SDL_KEYDOWN :
      case SDL_KEYUP :
      {
        auto& KE = e.key;
        auto code = impl::from_sdl_keycode(KE.keysym);
        vis( keyboard_event{code, KE.type == SDL_KEYDOWN} );
        break;
      }
    
      case SDL_WINDOWEVENT :
      {
        break;
      }
      
      case SDL_QUIT :
        want_quit = true;
        break;
      
      default :
        break;
    }
  }
  
  bool is_active(key_modifier mod) const {
    return SDL_GetModState() & (SDL_Keymod) mod;
  }
  
  ~sdl_backend()
  {
    SDL_Quit();
  }
  
  private : 
  
  static vec2f pos(int x, int y){
		return vec2f{ (float)x, (float)y };
	}
	
	static mouse_button to_mouse_button(int button) {
	  switch(button)
	  {
	    case SDL_BUTTON_LEFT : return mouse_button::left;
	    case SDL_BUTTON_MIDDLE : return mouse_button::middle;
	    case SDL_BUTTON_RIGHT : 
	      default : 
	      return mouse_button::right;
	  }
	}
};

} // impl