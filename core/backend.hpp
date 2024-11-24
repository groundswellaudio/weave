
#pragma once

#include <SDL.h>
#include <glad/glad.h>

#include "events/mouse_events.hpp"
#include "events/keyboard.hpp"

class sdl_backend
{
  bool mouse_is_dragging = false;
  int last_mouse_down = 0;
  
  public :
  
  bool want_quit = false;
  
  sdl_backend()
  {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    {
      fprintf(stderr, "failed to initialize SDL, aborting.");
      exit(1);
    }
  
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  
    // setting up a stencil buffer is necessary for nanovg to work
    SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 1 );
  }
  
  void load_opengl()
  {
    if (!gladLoadGLLoader((GLADloadproc) SDL_GL_GetProcAddress))
		{
			//fprintf(stderr, "failed to initialize the OpenGL context.");
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
      
      case SDL_KEYDOWN :
      case SDL_KEYUP :
      {
        auto& KE = e.key;
        auto code = impl::from_sdl_keycode(KE.keysym);
        vis( keyboard_event{code, KE.type == SDL_KEYDOWN} );
      }
    
      case SDL_WINDOWEVENT :
        break;
    
      case SDL_QUIT :
        want_quit = true;
        break;
      
      default :
        break;
    }
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