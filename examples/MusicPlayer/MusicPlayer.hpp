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
      trigger_button{"Cancel", &event_context::pop_overlay}    
    }
  }.background(rgb_f(colors::gray) * 0.3);
  
  auto w = ec.build_view<State>(dialog);
  w.do_layout({300, 150});
  w.set_position(ec.context().window().size() / 2 - w.size() / 2);
  ec.push_overlay(std::move(w));
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

template <class T>
using view_state = std::unique_ptr<T>;

using track_selection = widgets::table::selection_t;

auto track_info_menu(State& state, track_selection selected) {
  return views::text{"todo"};
  /* 
  return vstack {
    for_each( Database::track_properties(), [id, k = 0, selected] (auto key) mutable {
      auto setter = [id, key, selected] (auto& s, auto&& str) {
        traverse( selected, [&] (auto id) {
          s.set_track_property(id, key, str);
        });
      };
      return with_label{key.name(), text_field{p.value, setter}};
    });
  }; */ 
}

auto song_table_popup_menu(event_context& ec, track_selection selected) {
  widgets::popup_menu menu;
  auto info = [selected] (event_context& ec) {
    auto w = ec.build_view<State>(track_info_menu(ec.state<State>(), selected));
    ec.push_overlay(w);
  };
  auto add_to_playlist = [selected] (event_context& ec) {
    widgets::popup_menu m;
    int k = 0;
    for (auto& p : ec.state<State>().playlists()) {
      m.add_element(p.name, [pid = k++, selected] (event_context& ec) {
        selected.traverse([&] (auto id) {
          ec.state<State>().add_to_playlist(pid, id);
        });
      });
    }
    return m;
  };
  
  menu.add_element("Info", info);
  menu.add_element("Add to playlist", add_to_playlist);
  return menu;
}

struct LibraryView {
  
  struct songs_t {};
  struct artists_t {};
  struct albums_t {};
  struct artist_id { int value; };
  struct playlist_id { int value; };
  struct album_id {int value;};
  
  struct ViewState {
    selection_value<variant<songs_t, artists_t, albums_t, playlist_id, album_id>> selection;
    int artist = -1;
  };
  
  using view_state = std::unique_ptr<ViewState>;
  
  auto init_state() {
    return view_state( new ViewState{songs_t{}} );
  }
  
  auto left_panel(State& state, view_state& self) {
    using namespace views;
    
    auto setter = [p = self.get()] (auto v) { return p->selection.setter(v); };
    
    auto panel_item = [setter] (auto&& str, auto id) {
      return selectable{text{str}, setter(id)};
    };
    
    return vstack {
      text{"Library"},
      panel_item("Songs", songs_t{}),
      panel_item("Artists", artists_t{}),
      panel_item("Albums", albums_t{}),
      text{"Playlists"}, 
      for_each(state.playlists(), [this, p = 0, panel_item] (auto& playlist) mutable {
        return panel_item(playlist.name, playlist_id{p++});
      }),
      button{ "Create playlist", &State::add_playlist }
      /* 
      text{"Tags"},
      for_each(state.tags(), [] (auto& p) {
        return selectable{ text{p.name()}, group };
      }); */ 
    }.fill();
  }
  
  /* 
  auto artist_list(State& state) {
    return list{ state.artists(), false }.on_select_cell(&State::set_current_artist);
  } */ 
  
  auto body(State& state, view_state& self) {
    using namespace views;
    
    bool update_table = std::exchange(state.table_mutated, false);
    
    auto center_view = either {
      self->selection.value, 
      [&] (songs_t) {
        return table{state.database.tracks, update_table}
                    .on_cell_double_click(&State::play_track)
                    .on_file_drop(&on_file_drop)
                    .popup_menu(&song_table_popup_menu);
      },
      [&] (artists_t) {
        return list{state.artists(), false};
      },
      [&] (albums_t) {
        return flow{ 
          400,
          for_each(state.database.albums, [p = self.get(), k = 0] (auto& a) mutable {
            auto setter = p->selection.setter(album_id{k++});
            return vstack {
              on_click{views::image{a.cover, false}.fit({100, 100}), setter},
              text{a.name}
            };
          })
        };
      },
      [&] (playlist_id id) {
        return text{"todo"};
        /* 
        return table{ state.database.playlist(id.value), false }
               .on_cell_double_click( &State::play_track ); */ 
      },
      [&] (album_id id) {
        return vstack {
          views::image{state.database.album(id.value).cover, false}.fit({600, 600}),
          text{state.database.album(id.value).name},
          table{state.database.album_tracks(id.value), false}.on_cell_double_click(&State::play_track)
        };
      }
    };
    
    return hstack{left_panel(state, self), center_view};
  }
};

/// A composite view is a composition of a view state and a body() function 
/// which is a product of both this view state and the global state.
template <class T, class State>
struct composite_view : view<composite_view<T, State>> {
  
  using view_state_t = typename T::view_state;
  
  using body_t = decltype(std::declval<T&>().body(std::declval<State&>(), std::declval<view_state_t&>()));
  
  composite_view(auto&&... args) : definition{args...} {}
  composite_view(composite_view&&) = default;
  
  auto build(const widget_builder& ctx, State& state) {
    view_state = definition.init_state();
    definition_body.reset(new optional<body_t>{definition.body(state, view_state)});
    return (**definition_body).build(ctx, state);
  }
  
  auto rebuild(auto& old, widget_ref r, auto&& up, auto& state) {
    // Move the value
    auto old_body = std::move(**old.definition_body);
    // Move the pointer
    definition_body = std::move(old.definition_body);
    view_state = std::move(old.view_state);
    definition_body->emplace(definition.body(state, view_state));
    return (**definition_body).rebuild(old_body, r, up, state);
  }
  
  T definition;
  view_state_t view_state;
  std::unique_ptr<optional<body_t>> definition_body;
};

using library_view = composite_view<LibraryView, State>;

auto make_view(State& state)
{
  using namespace views;
  
  state.check_done_reading();
  
  bool update_cover = std::exchange(state.current_cover_updated, false);
  
  return vstack{ top_panel(state),
                 hstack{ 
                        library_view{}, 
                        views::image{state.current_cover, update_cover}
                        .fit({600, 600})
                        }.align(1).fill()
               }.align_center()
                .margin({10, 10})
                .background( rgb_f(colors::gray) * 0.4 );
}

inline void run_app() {
  State state;
  window_properties prop;
  prop.name = "spinner";
  prop.size = vec2f{600, 400};
  auto app = make_app(state, &make_view, prop);
  app.run(state);
}