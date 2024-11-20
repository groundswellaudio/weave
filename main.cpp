#include "spiral.hpp"
#include <ranges>
#include <shared_mutex>
#include <mutex>

#define MINIAUDIO_IMPLEMENTATION
#include "deps/miniaudio/miniaudio.h"

struct AppState : audio_renderer<AppState> {
  
  void render_audio(audio_output_stream os) 
  {
  }
  
  float x = 0.3, y = 0.5, z = 0.9;
  bool flag = false;
  std::vector<float> vals = {0.2, 0.3, 0.4};
};

struct PmSynth : audio_renderer<PmSynth>, app_state
{
  static constexpr int num_osc = 4;
  
  auto read_scope() { return std::shared_lock{mut}; }
  auto write_scope() { return std::unique_lock{mut}; }
  
  void render_audio(audio_output_stream os)
  {
    auto _ = read_scope();
    
    for (auto s : os) 
    {
      float res = 0;
      int OscIndex = 0;
      for (auto& o : osc)
      {
        float acc = 0;
        for (int k = 0; k < num_osc; ++k)
          acc += osc[k] * mod_matrix[OscIndex][k];
        o = std::sin( osc_phase[OscIndex] * 2 * 3.14 * 440 * freq_ratio[OscIndex] + acc );
        osc_phase[OscIndex] += 1.f / 44100.f;
        res += o * osc_vol[OscIndex];
        ++OscIndex;
      }
      s[0] = res;
      s[1] = res;
    }
  }
  
  std::shared_mutex mut;
  float mod_matrix[num_osc][num_osc] = {0};
  float osc_phase[num_osc] {0};
  float osc[num_osc] = {0.0};
  float freq_ratio[num_osc] = {1.0};
  float osc_vol[num_osc] = {1.0, 0.0};
  
  bool show_modulations;
};

auto make_demo_app(AppState& state)
{
  auto s = scrollable {
    vec2f{ 60, 50 },
    vstack {
      slider{ %lens(^(state.x)) },
      slider{ %lens(^(state.y)) }, 
      slider{ %lens(^(state.z)) }
    }.with_margin({20, 20})
     .with_interspace(30)
  };
  return s;
};

struct numeric_field_properties {
  float min, max;
};

struct numeric_field_widget : widget_base 
{
  using prop_t = numeric_field_properties;
  
  numeric_field_widget(prop_t prop) : prop{prop} {}
  
  numeric_field_properties prop;
  float mult_mod;
  
  using Self = numeric_field_widget;
  using value_type = float;
  
  void on(mouse_event e, event_context<Self>& ec) {
    if (e.is_mouse_down())
      mult_mod = (1.f - 3 * e.position.x / size().x);
    else if (e.is_mouse_drag())
    {
      auto delta = - std::pow(10, mult_mod) * e.mouse_drag_delta().y;
      auto NewVal = std::clamp<float>( ec.read() + delta, prop.min, prop.max );
      ec.write( NewVal );
    }
  }
  
  void paint(painter& p, float value) {
    p.stroke_style(colors::red);
    p.stroke_rect({0, 0}, size());
    p.font_size(13.f);
    auto str = std::format("{:.2f}", value);
    p.text_alignment(text_align::x::center, text_align::y::center);
    p.text( sz / 2, str );
  }
  
  auto layout() const { return vec2f{50, 30}; }
};

template <class L>
using freq_ratio = simple_view_for<numeric_field_widget, L, numeric_field_properties>;

#define MLens(Postfix) [=] (auto& s) -> decltype(auto) { return (s Postfix); }

auto make_pm_synth(PmSynth& state)
{
  auto slider_row = [&] (int row) {
    return hstack {
      for_each{
        iota(state.num_osc),
        [row] (int col) {
          return slider{ MLens(.mod_matrix[row][col]) };
        }
      }
    }.with_interspace(10);
  };
  
  auto mod_matrix = vstack{ 
    for_each{ 
      iota(state.num_osc),
      slider_row
    }
  }.with_interspace(10);
  
  auto osc_panel_row = [] (int row) {
    return hstack {
      freq_ratio { MLens(.freq_ratio[row]), numeric_field_properties{0, 100} },
      slider { MLens(.osc_vol[row]) }
    };
  };
  
  auto left_panel = vstack {
    for_each { 
      iota(state.num_osc),
      osc_panel_row
    }
  }.with_interspace(10);
  
  return hstack {
    left_panel, mod_matrix
  }.with_interspace(30).with_margin({30, 30});
}

int main()
{
  PmSynth state;
  state.start_audio_render();
  auto app = make_app(state, &make_pm_synth);
  app.run(state);
}