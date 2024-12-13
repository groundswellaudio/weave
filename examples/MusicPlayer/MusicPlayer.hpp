#pragma once

#include "spiral.hpp"
#include "Model.hpp"

void paint_play_button(painter& p, bool flag, vec2f sz) {
  p.fill_style(colors::white);
  if (!flag)
    p.triangle({0, 0}, {0, sz.y}, {sz.x, sz.y / 2});
  else {
    auto rsz = vec2f{sz.x / 5, sz.y};
    p.rectangle({1 * sz.x / 5, 0}, rsz);
    p.rectangle({3 * sz.x / 5, 0}, rsz);
  }
}

template <bool Direction>
void paint_transport_button(painter& p, vec2f sz) {
  p.fill_style(colors::white);
  
  auto traii = p.translate({3, 3});
  sz -= 3;
  
  if (Direction) {
    p.triangle({0, 0}, {0, sz.y}, sz / 2);
    p.triangle({sz.x / 2, 0}, {sz.x / 2, sz.y}, {sz.x, sz.y / 2});
  }
  else {
    auto a = vec2f{0, sz.y / 2};
    auto b = vec2f{sz.x / 2, 0};
    auto c = vec2f{sz.x / 2, sz.y};
    p.triangle(a, b, c);
    auto o = vec2f{sz.x / 2, 0};
    p.triangle(a + o, b + o, c + o);
  }
}

auto top_panel(State& state)
{
  using namespace views;
  
  return hstack {
    hstack {
      graphic_trigger_button{&paint_transport_button<false>, &State::previous_track},
      graphic_toggle_button{&paint_play_button, state.is_playing, &State::set_play}.size({20, 20}),
      graphic_trigger_button{&paint_transport_button<true>, &State::next_track}
    },
    text{state.current_track_name()},
    slider{ [] (auto& s) -> auto& { return s.player.volume; } }
  }.interspace(30);
}

void on_file_drop(event_context& ec, const std::string& path_str) {
  auto path = fs::path(path_str);
  if (!is_directory(path)) {
    ec.state<State>().load_track(path);
    return;
  }
  
  auto yes = [path] (event_context& ec) { ec.state<State>().load_directory(path); ec.pop_overlay(); };
  
  using namespace views;
  
  auto dialog = vstack{ 
    text{"Import all audio files from directory?"},
    hstack{
      trigger_button{"Yes", yes},
      trigger_button{"Cancel", [] (event_context& ec) {ec.pop_overlay();}},
    }
  }.background(rgb_f(colors::gray) * 0.3);
  
  auto w = ec.build_view<State>(dialog);
  w.set_position(ec.context().window().size() / 2 - w.size() / 2);
  ec.push_overlay(std::move(w));
}

auto make_view(State& state)
{
  using namespace views;
  
  /* 
  auto on_file_drop = [] (event_context_t<void>& ec, const std::string& path) {
    if (is_directory(ghc::path{path})
      std::cout << "got directory" << std::endl;
    ec.state<State>().load_track()
  }; */ 
  
  /* selection_panel {
    
  vstack{
    text{"Songs"}, 
    text{"Artists"},
    text{"Album"},
    spacer{}
    
  } */ 
  
  state.check_done_reading();
  
  bool update_cover = std::exchange(state.current_cover_updated, false);
  
  auto side_panel = list{{"Songs", "Artists", "Albums"}, false}
                     .on_select_cell( &State::set_presentation );
  
  bool update_table = std::exchange(state.table_mutated, false);
  
  auto songs_table = table{ state.database.tracks, update_table }
                      .on_cell_double_click( &State::play_track )
                      .on_file_drop(&on_file_drop);
  
  auto center_view = either {
    state.presentation_kind,
    songs_table,
    list{ state.artists(), false }.on_select_cell(&State::set_current_artist)
    // maybe{ state.current_artist, artist_discography(state) }
    //list{ state.albums(), false }.on_select_cell(&State::set_current_album)
  };
  
  return vstack{ top_panel(state),
                 hstack{ 
                        hstack{
                          side_panel,
                          center_view
                        }.align(0),
                        views::image{state.current_cover, update_cover}
                        .fit({300, 300})
                        }.align(1)
               }.align_center()
                .margin({10, 10})
                .background( rgb_f(colors::gray) * 0.4 );
}

inline void run_app() {
  State state;
  auto app = make_app(state, &make_view);
  app.run(state);
}