#pragma once

#include "weave.hpp"
#include "Model.hpp"

using namespace weave;

struct transport_bar_widget : widget_base {
  
  // the bar is in the last 10pix
  static constexpr float header = 15;
  static constexpr float bar_thickness = 8;
  
  float ratio_value = 0;
  unsigned max_value = 0;
  write_fn<float> write;
  
  void on(mouse_event e, event_context& ec) {
    if (e.position.y > header && e.is_down() || e.is_drag()) {
      // write_value(e.position.x / size().x);
      ratio_value = e.position.x / size().x;
      write(ec, ratio_value);
    }
  }
  
  auto size_info() const {
    widget_size_info res;
    res.min = point{100, 30};
    res.max = point{infinity<float>(), 30};
    res.nominal_size = point{120, 30};
    res.flex_factor = point{1, 0};
    return res;
  }
  
  void set_value(float val) {
    ratio_value = val;
  }
  
  void paint(painter& p) const {
    unsigned elapsed = ratio_value * max_value;
    unsigned remaining = max_value - elapsed; 
    
    p.fill_style(colors::white);
    p.font_size(11);
    
    p.text_align(text_align::x::left, text_align::y::center);
    p.text( point{5, header / 2}, std::format("{:02}:{:02}", elapsed / 60, elapsed % 60) );
    
    // Workaround text align right not working yet
    //p.text_align(text_align::x::right, text_align::y::center);
    p.text( point{size().x - 33, header / 2}, std::format("-{:02}:{:02}", remaining / 60, remaining % 60));
    
    p.fill_style(colors::gray);
    p.fill(rounded_rectangle(rectangle({0, header}, {size().x, bar_thickness}), 5)); 
    p.fill_style( rgba_f{colors::cyan} );
    rectangle r { {0, header}, {ratio_value * size().x, bar_thickness} };
    p.fill(rounded_rectangle(r, 5));
  }
};

template <class Fn>
struct transport_bar_view : view<transport_bar_view<Fn>>, views::view_modifiers {
  
  using widget_t = transport_bar_widget;
  
  transport_bar_view(Fn fn, int length) : write_fn{fn}, track_length{length} {}

  template <class S>
  auto build(const build_context& ctx, S& state) {
    auto res = widget_t{};
    res.write = [fn = write_fn] (event_context& ec, auto val) {
      context_invoke<S>(fn, ec, val);
    };
    res.max_value = track_length; 
    return res;
  }
  
  rebuild_result rebuild(transport_bar_view<Fn> old, widget_ref r, const build_context& ctx, ignore) {
    r.as<widget_t>().max_value = track_length;
    return {};
  }
  
  Fn write_fn;
  int track_length; 
};

void paint_play_button(painter& p, bool flag, point sz) {
  p.fill_style(colors::white);
  if (!flag) {
    p.fill(triangle(circle(sz / 2, sz.x / 2), radians{0.f}));
    // p.fill(triangle({0, 0}, {0, sz.y}, {sz.x, sz.y / 2}));
  }
  else {
    auto rsz = vec2f{sz.x / 5, sz.y};
    p.fill( rectangle(rsz).translated({1 * sz.x / 5, 0}) );
    p.fill( rectangle(rsz).translated({3 * sz.x / 5, 0}) );
  }
}

template <bool Direction>
void paint_transport_button(painter& p, point sz) {
  p.fill_style(colors::white);
  
  auto _ = p.translate({3, 3});
  sz -= 3;
  
  if (Direction) {
    auto tri = triangle({0, 0}, {0, sz.y}, sz / 2);
    p.fill( tri );
    p.fill( tri.translated({sz.x / 2, 0}) );
  }
  else {
    auto a = vec2f{0, sz.y / 2};
    auto b = vec2f{sz.x / 2, 0};
    auto c = vec2f{sz.x / 2, sz.y};
    auto tri = triangle{a, b, c};
    
    p.fill(tri);
    p.fill(tri.translated({sz.x / 2, 0}));
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
      button{"Yes", yes},
      button{"Cancel", &event_context::pop_overlay}    
    }
  }
  .background(rgb_f(colors::gray) * 0.3)
  .align_center()
  .margin({30, 30});
  
  auto w = dialog.build(build_context{ec.context()}, ec.state<State>());
  w.do_layout({300, 100});
  w.set_position(ec.context().window().size() / 2 - w.size() / 2);
  ec.push_overlay(std::move(w));
}

auto top_panel(State& state)
{
  using namespace views;
  
  auto transport_buttons = hstack {
      graphic_button{&paint_transport_button<false>, &State::previous_track}.with_fixed_size({30, 30}),
      graphic_toggle_button{&paint_play_button, state.is_playing(), &State::set_play}.with_fixed_size({30, 30}),
      graphic_button{&paint_transport_button<true>, &State::next_track}.with_fixed_size({30, 30})
    }.align_center();
  
  auto transport_bar = transport_bar_view{ [] (State& s, float val) { s.set_track_position(val); }, 
                                          state.current_track_length() }
    .animate_when(state.is_playing(), 1000, 
    [] (auto& w, animation_context& ctx) { 
      w.set_value(ctx.state<State>().current_track_position());
      return true;
    });
  
  auto device_val = readwrite( state.player.current_device_index(), 
                               [] (State& s, int id) { s.player.set_audio_device(id); });
  
  return hstack {
    combo_box(device_val, audio_output_devices()), 
    transport_buttons,
    vstack{ either{ state.current_track(), 
              [] (auto& t) { return text{"{} - {}", t.artist(), t.title()}.align_center(); }, 
              [] () { return text{"Not playing."}.align_center(); }
            },
            transport_bar 
    }.align_center(),
    slider{ [] (auto& s) -> auto& { return s.player.volume; } }
      .space(scalar_space::decibel())
      .range(-80, 0)
      .write_scaled(false)
  }.interspace(30).align_center()
  .margin({20, 10});
}

using track_selection = widgets::table::selection_t;

auto track_info_menu(State& state, track_selection selected) {
  return views::text{"todo"};
  /* 
  return vstack {
    with_label{"Title", text_field{}},
    with_label{"Artist", text_field{}}
  }; */ 
  
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
    auto& s = ec.state<State>();
    auto w = track_info_menu(s, selected).build(build_context{ec.context()}, s);
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
      }, ec.graphics_context());
    }
    return m;
  };
  
  menu.add_element("Info", info, ec.graphics_context());
  menu.add_element("Add to playlist", add_to_playlist, ec.graphics_context());
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
  
  using view_state = ViewState;
  
  auto init_state() {
    return ViewState{songs_t{}};
  }
  
  auto left_panel(State& state, view_state& self) {
    using namespace views;
    
    auto setter = [&self] (auto v) { return self.selection.setter(v); };
    
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
      self.selection.value, 
      [&] (songs_t) {
        return table{state.database.tracks, update_table}
                    .on_cell_double_click(&State::play_track)
                    .popup_menu(&song_table_popup_menu)
                    .on_file_drop(&on_file_drop);
      },
      [&] (artists_t) {
        return list{state.artists(), false};
      },
      [&] (albums_t) {
        return flow{ 
          400,
          for_each(state.database.albums, [&self, k = 0] (auto& a) mutable {
            auto setter = self.selection.setter(album_id{k++});
            return vstack {
              views::image{a.cover, false}.fit({100, 100}).on_click([setter] (ignore, ignore) {setter();}),
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

using library_view = weave::views::composite_view<LibraryView, State>;

auto make_view(State& state)
{
  using namespace views;
  
  state.check_done_reading();
  
  bool update_cover = std::exchange(state.current_cover_updated, false);
  
  return vstack{ top_panel(state),
                  hstack{ 
                        library_view{}, 
                        views::image(state.current_cover, update_cover)
                        .fit({600, 600})
                        }.align(1).fill() 
               }.align_center()
                .margin({10, 10})
                .background( rgb_f(colors::gray) * 0.4 )
                .fill(); 
}

inline void run_app() {
  State state;
  window_properties prop;
  prop.name = "spinner";
  prop.size = vec2f{600, 400};
  auto app = make_app(state, &make_view, prop);
  app.run(state);
}