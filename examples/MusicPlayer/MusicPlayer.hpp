#pragma once

#include "weave.hpp"
#include "Model.hpp"

#include <ranges>
#include <algorithm>

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
    res.nominal = point{120, 30};
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
    auto res = widget_t{ctx.new_id()};
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
  
  auto yes = [path] (event_context& ec, widget_id id) { 
    ec.state<State>().load_directory(path); 
    ec.pop_overlay(id);
  };
  
  using namespace views;
  
  auto dialog = vstack{ 
    text{"Import all audio files from directory?"},
    hstack{
      button{"Yes", yes},
      button{"Cancel", &event_context::pop_overlay}    
    }
  }
  .background_color(rgb_f(colors::gray) * 0.3)
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
              [] (auto& t) { return text{"{} - {}", std::string(t.artist()), std::string(t.title())}.align_center(); }, 
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
  
  using namespace weave::views;
  
  auto MakeField = [&] (auto setter, auto property) {
  
    auto Field = text_field{setter};
    
    auto HasMultipleValues = [&] () {
      auto i = selected.front();
      for (auto t : selected) {
        if (property(state.track(t)) != property(state.track(i)))
          return true;
      }
      return false;
    };
    
    if (selected.size() > 1 && HasMultipleValues())
      Field.set_background_text("Multiple values");
    else
      Field.set_value(property(state.track(selected.front()))); 
  
    return Field;
  };
  
  auto TitleField = MakeField( 
    [selected] (auto& state, std::string_view str) { state.set_tracks_title(selected, str); }, 
    [] (auto& t) -> auto&& { return t.title(); } 
  );
  
  auto ArtistField = MakeField( 
    [selected] (auto& state, std::string_view str) {state.set_artist_name(selected, str);},
    [] (auto& t) -> auto&& { return t.artist(); } 
  );
  
  auto AlbumField = MakeField(
    [selected] (auto& state, std::string_view str) {state.set_album_name(selected, str);},
    [] (auto& t) -> auto&& { return t.album(); } 
  );
  
  return vstack {
    with_label{"Title", TitleField},
    with_label{"Artist", ArtistField},
    with_label{"Album", AlbumField}, 
    button{"Close", &event_context::pop_overlay}
  }.background_color(rgb_f(colors::gray) * 0.3)
  .align_center()
  .margin({40, 40})
  .interspace(10);
}

auto song_selection_popup_menu(event_context& ec, track_selection selected) {
  widgets::popup_menu menu {ec.tree().new_id()};
  auto info = [selected] (event_context& ec) {
    auto& s = ec.state<State>();
    auto w = track_info_menu(s, selected).build(build_context{ec.context()}, s);
    w.do_layout(w.size_info().nominal);
    w.set_position(ec.context().window().size() / 2 - w.size() / 2);
    ec.push_overlay(std::move(w));
  };
  auto add_to_playlist = [selected] (event_context& ec) {
    widgets::popup_menu m {ec.tree().new_id()};
    int k = 0;
    for (auto& p : ec.state<State>().playlists()) {
      m.add_element(p.name, [pid = k++, selected] (event_context& ec) {
        for (auto id : selected)
          ec.state<State>().add_to_playlist(pid, id);
      }, ec.graphics_context());
    }
    return m;
  };
  
  menu.add_element("Info", info, ec.graphics_context());
  menu.add_element("Add to playlist", add_to_playlist, ec.graphics_context());
  return menu;
}

struct songs_t { bool operator==(const songs_t&) const = default;};
struct artists_t { bool operator==(const artists_t&) const = default; };
struct albums_t { bool operator==(const albums_t&) const = default;};
struct artist_id { int value; bool operator==(const artist_id&) const = default; };
struct playlist_id { int value; bool operator==(const playlist_id& p) const = default;};
struct album_id { std::string_view artist, title; bool operator==(const album_id&) const = default; };

// The artist name strings are uniqued
struct artist_key {
  bool operator==(const artist_key& o) const { return value.data() == o.value.data(); }
  std::string_view value;
};
  
struct CenterViewState {
  variant<songs_t, artists_t, albums_t, playlist_id, album_id> selection;
  optional<artist_key> artist;
  std::set<unsigned> song_selection;
};

template <class R, class A, class B>
auto filter_from_ascending_indices(A&& range, B&& indices) -> R {
  // A filter + to<std::vector> doesn't work for this because the mutable closure will be 
  // evaluated several times
  R res;
  res.reserve(indices.size());
  auto i = 0;
  auto idx_it = indices.begin();
  for (auto&& e : range) {
    if (idx_it == indices.end())
      return res;
    if (i++ == *idx_it) {
      ++idx_it;
      res.push_back(e);
    }
  }
  return res;
}

template <class TrackRange>
struct make_track_view {
  State& state;
  CenterViewState& view_state;
  TrackRange track_range;
  int p = 1;
  
  auto operator()(tuple<const Database::Track&, int> TrackAndId) {
    using namespace weave::views;
    auto [t, tid] = TrackAndId;
    auto play_track = [id = tid] (State& state, bool Val) { 
      if (Val)
        state.play_track(id);
      else
        state.set_play(false); 
      return Val;
    };
    auto r = hstack{ text{"{} - ", (int)p}.with_flex_factor({0,0}), 
                     graphic_toggle_button{&paint_play_button, 
                                           state.is_playing() && state.current_track_id == tid, 
                                           play_track}
                                          .with_fixed_size({11, 11}),
                     text{t.title()}.align_right() }.align_center();
    return multi_selectable{r, &view_state.song_selection, p++}
           .with_popup_menu([s = &view_state, track_range = track_range] (event_context& ec) {
              auto track_id_range = std::ranges::transform_view(track_range, [] (auto&& e) { return get<1>(e); });
              auto selected_tracks = filter_from_ascending_indices<std::vector<unsigned>>(track_id_range, s->song_selection);
              return song_selection_popup_menu(ec, selected_tracks);
           });
  }
  
 };

struct make_album_view {
  State& state;
  CenterViewState& view_state;
  
  auto operator()(const Database::Album& a) {
    using namespace weave::views;
    auto album_tracks = state.database.album_tracks(a);
    return vstack {
      hstack{ views::image{a.cover}.fit({150, 150}), text{a.title()}.font_size(20) }.align_center(),
      for_each(album_tracks, make_track_view{state, view_state, album_tracks})
    };
  }
};

struct LibraryView {
  
  using view_state = CenterViewState;
  
  auto init_state() {
    return view_state{songs_t{}};
  }
  
  auto left_panel(State& state, view_state& self) {
    using namespace views;
    
    auto panel_item = [&self] (auto&& str, auto id) {
      return selectable{text{str}, &self.selection, id};
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
    }.fill_cross_axis();
  }
  
  /* 
  auto artist_list(State& state) {
    return list{ state.artists(), false }.on_select_cell(&State::set_current_artist);
  } */ 
  
  auto body(State& state, view_state& self) {
    using namespace views;
    
    auto center_view = either {
      self.selection, 
      [&] (songs_t) {
        return table{state.database.tracks}
                    .on_cell_double_click(&State::play_track)
                    .popup_menu(&song_selection_popup_menu)
                    .on_file_drop(&on_file_drop);
      },
      [&] (artists_t) {
        auto left = vstack {
          for_each{ state.artists(), [&self] (auto& a) mutable {
            auto t = text{a.name}.font_size(20)
                      .background(views::rectangle{}.stroke(1).color(colors::white));
            return selectable{t, &self.artist, artist_key{std::string_view{a.name}}};
          }}
        }.interspace(0).scrollable().with_nominal_size({200, 300});
        auto artist_view = [&] (artist_key current) {
          return vstack{
            text{current.value}.font_size(30),
            for_each( state.database.artist_albums(current.value),
                      make_album_view{state, self} )
          }.with_nominal_size({300, 300}).scrollable();
        };
        
        auto right = either{self.artist, artist_view, [] () { return vstack{}; }};
        return hstack{left, right};
      },
      [&] (albums_t) {
        return flow{ 
          400,
          for_each(state.database.albums, [&self, k = 0] (auto& a) mutable {
            auto setter = [&self, artist_name = std::string_view{a.artist}, name = std::string_view{a.name}] (ignore, ignore) { 
              self.selection = album_id{artist_name, name}; 
            };
            return vstack {
              views::image{a.cover}.fit({150, 150}).on_click(setter),
              text{a.name}.with_nominal_size({150, 20}),
              text{a.artist}.with_nominal_size({150, 20})
            }.interspace(2);
          })
        }.rounded(6);
      },
      [&] (playlist_id id) {
        auto&& tracks = state.database.playlist_tracks(id.value);
        return vstack {
          text_field{ [id] (State& s, std::string_view str) { s.set_playlist_name(id.value, str); } }
          .set_value(state.database.playlist(id.value).name).font_size(30),
          for_each( tracks, make_track_view{state, self, tracks} )
        }.scrollable();
      },
      [&] (album_id id) {
        return make_album_view{state, self}(state.database.album(id.artist, id.title))
               .scrollable().with_nominal_size({300, 300});
      }
    };
    
    return hstack{left_panel(state, self), 
                  center_view, 
                  views::image(state.current_cover).fit({600, 600})
                  }.fill_cross_axis();;
  }
};

using library_view = weave::views::composite_view<LibraryView, State>;

auto make_view(State& state)
{
  using namespace views;
  
  state.check_done_reading();
  
  return vstack{ top_panel(state),
                  library_view{}, 
               }.align_center()
                .margin({10, 10})
                .background_color( rgb_f(colors::gray) * 0.4 )
                .fill_cross_axis(); 
}

inline void run_app() {
  State state;
  window_properties prop;
  prop.name = "spinner";
  prop.size = point{600, 400};
  auto app = make_app(state, &make_view, prop);
  app.run(state);
}