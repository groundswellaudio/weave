#pragma once

#include "spiral.hpp"

#include <ranges>
#include <random>
#include <future>

struct observe_token {
  bool operator==(const observe_token& o) const = default;
  int value;
};

template <class T>
struct observed : T {
  
  observed& operator=(T&& x) {
    T::operator=((T&&)x);
    note_mutation();
    return *this;
  } 
   
  void note_mutation() { ++observe_gen; }
  observe_token token() const { return {observe_gen}; }
  
  int observe_gen = 0;
};

template <class T>
bool is_up_to_date(observe_token o, const observed<T>& obs) {
  return obs.token() == o;
}

struct State : audio_renderer, app_state {
  
  void load_audio() {
    stop_render();
    auto path = open_file_dialog();
    if (!path)
      return;
    audio = load_audio_file(*path);
    start_render();
  }
  
  void render_audio(audio_output_buffer& os) {
    
  }
  
  audio_buffer audio;
  bool waveform_changed = false;
};

struct waveform_widget : widget_base {
  
  std::vector<vec2f> data;
  
  void paint(painter& p) {
    p.fill_style(colors::black);
    p.rectangle({0, 0}, size());
    p.stroke_style(colors::white);
    p.fill_style(colors::white);
    float posx = 0;
    for (auto f : data) {
      f = (f + 1) / 2;
      p.line({posx, f.x * size().y}, {posx, f.y * size().y}, 1);
      posx++;
    }
  }
  
  void on(input_event e, ignore) {}
};

template <class T>
struct audio_buffer_view : view<audio_buffer_view<T>> {
  observed<T>& data; 
  observe_token ot;
  
  audio_buffer_view(observed<T>& data) : data{data} {}
  
  auto build(auto&& builder, ignore) {
    auto res = audio_buffer_widget{{400, 200}};
    update_display_data(res);
    return res;
  }
  
  void update_display_data(audio_buffer_widget& res) {
    res.data.clear();
    auto size = res.size();
    res.data.reserve(size.x);
    auto samplesPerPx = data.size() / size.x; 
    for (int k = 0; k < data.size() - samplesPerPx; k += samplesPerPx) {
      auto r = std::span{data.begin() + k, data.begin() + k + samplesPerPx};
      auto [min, max] = std::ranges::minmax(r);
      res.data.push_back({min, max});
    }
  }
  
  rebuild_result rebuild(audio_buffer_view& Old, widget_ref elem, auto&& updater, ignore) {
    ot = Old.ot;
    if (is_up_to_date(ot, data))
      return {};
    update_display_data(elem.as<audio_buffer_widget>());
    ot = data.token();
    return {};
  }
};

auto make_view(State& state)
{
  using namespace views;
  
  return vstack {
    trigger_button { "Load texture", [] (auto& s) { s.load_audio(); } },
    audio_buffer_view{ state.audio }
  };
}

inline void run_app() {
  State state;
  auto app = make_app(state, &make_view);
  app.run(state);
}