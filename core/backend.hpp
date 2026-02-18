
#pragma once

#include <SDL3/SDL.h>
#include <glad/glad.h>

#include "events/mouse_events.hpp"
#include "events/keyboard.hpp"
#include "window.hpp"

#include <iostream>
#include <chrono>

namespace weave {

namespace impl {

class sdl_backend
{
  bool mouse_is_dragging = false;
  std::chrono::time_point<std::chrono::steady_clock> last_mouse_down;
  bool has_resized = false;
  
  public :
  
  bool want_quit = false;
  
  template <class Ctx>
  static bool on_window_resize(void* data, SDL_Event* event)
  {
    if (event->type == SDL_EVENT_WINDOW_RESIZED)
      ((Ctx*)data)->on_window_resize();
    return true;
  }
  
  template <class Ctx>
  sdl_backend(Ctx* ctx)
  {
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
      fprintf(stderr, "failed to initialize SDL, aborting.");
      exit(1);
    }
  
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  
    // setting up a stencil buffer is necessary for nanovg to work
    SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 1 );
    
    SDL_AddEventWatch( &on_window_resize<Ctx>, ctx );
  }
  
  void start_text_input(window& win) {
    SDL_StartTextInput(win.get());
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
      case SDL_EVENT_MOUSE_BUTTON_DOWN :
      {
        using namespace std::chrono_literals;
        auto time = std::chrono::steady_clock::now();
        bool is_double_click = false;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(time - last_mouse_down) 
              < 250ms) {
          last_mouse_down = time;
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
      
      case SDL_EVENT_MOUSE_BUTTON_UP :
      {
        mouse_is_dragging = false;
        auto p = pos(e.button.x, e.button.y);
        vis( mouse_event{p, mouse_up{}} );
        break;
      }
      
      case SDL_EVENT_MOUSE_MOTION :
      {
        auto p = pos(e.motion.x, e.motion.y);
        auto delta = pos(e.motion.xrel, e.motion.yrel);
        vis( mouse_event{p, mouse_move{delta, mouse_is_dragging, to_mouse_button(e.button.button)}} );
        break;
      }
      
      case SDL_EVENT_MOUSE_WHEEL :
      {
        auto delta = pos(e.wheel.x, e.wheel.y);
        auto ev = mouse_event{pos(e.wheel.mouse_x, e.wheel.mouse_y), mouse_scroll{delta}};
        vis(ev);
        break;
      }
    
      case SDL_EVENT_TEXT_EDITING :
        break;
      case SDL_EVENT_TEXT_INPUT :
      {
        auto ev = keyboard_event{std::string(e.text.text)};
        vis(ev);
        break;
      }
      
      case SDL_EVENT_DROP_FILE: 
      {
        std::string file = e.drop.data;
        vis( mouse_event{vec2f{0, 0}, file_drop(std::move(file))} );
        break;
      }
      
      case SDL_EVENT_KEY_DOWN :
      case SDL_EVENT_KEY_UP :
      {
        auto& KE = e.key;
        auto code = impl::from_sdl_keycode(KE.key);
        vis( keyboard_event{tuple<keycode, bool>{code, KE.type == SDL_EVENT_KEY_DOWN}} );
        break;
      }
      
      case SDL_EVENT_QUIT :
        want_quit = true;
        break;
      
      case SDL_EVENT_WINDOW_FOCUS_GAINED:
      case SDL_EVENT_WINDOW_FOCUS_LOST:
      case SDL_EVENT_WINDOW_SHOWN:
      case SDL_EVENT_WINDOW_EXPOSED:
      case SDL_EVENT_WINDOW_OCCLUDED:
      case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
      case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
      case SDL_EVENT_WINDOW_MOUSE_ENTER:
      case SDL_EVENT_CLIPBOARD_UPDATE:
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

} // weave