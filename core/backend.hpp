
#pragma once

#include "events/mouse_events.hpp"
#include <SDL.h>
#include <glad/glad.h>

class sdl_backend
{
  bool mouse_is_dragging = false;
  
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
        mouse_is_dragging = true;
        auto ev = mouse_event{pos(e.button.x, e.button.y), mouse_down{to_mouse_button(e.button.button), false}};
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
        vis( mouse_event{p, mouse_move{{}, mouse_is_dragging, to_mouse_button(e.button.button)}} );
        break;
      }
      
      case SDL_MOUSEWHEEL :
        break;
    
      case SDL_TEXTEDITING :
      case SDL_TEXTINPUT :
      case SDL_KEYDOWN :
      case SDL_KEYUP :
        break;
    
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