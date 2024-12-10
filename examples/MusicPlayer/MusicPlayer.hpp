#pragma once

#include "spiral.hpp"

#include <taglib/tag.h>
#include <taglib/fileref.h>
#include <taglib/tpropertymap.h>

#include <ghc/filesystem.hpp>
#include <atomic>
#include <set>

struct TrackPlayer : audio_renderer<TrackPlayer>
{
  void render_audio(audio_output_stream& os) {
    if (head == -1)
      return;
    if (head + os.sample_size() > buffer.size()) {
      auto it = os.begin();
      for (; head < buffer.size(); head += 2)
      {
        auto s = *it;
        s[0] = buffer[head] * volume;
        s[1] = buffer[head + 1] * volume;
        ++it;
      }
      head = -1;
      done_reading = true;
    }
    else
    for (auto s : os)
    {
      s[0] = buffer[head] * volume;
      s[1] = buffer[head + 1] * volume;
      head += 2;
    }
  }
  
  std::atomic<float> volume;
  audio_buffer buffer;
  int head = -1;
  std::atomic<bool> done_reading = false;
};

static optional<image<rgba<unsigned char>>> read_file_cover(const std::string& path) {
  TagLib::FileRef f{path.c_str()};
  if (!f.tag())
    return {};
  auto pic_infos = f.complexProperties("PICTURE");
  if (pic_infos.begin() == pic_infos.end())
    return {};
  auto& pic = *pic_infos.begin();
  auto data_it = pic.find("data");
  if (data_it == pic.end())
    return {};
  auto&& data_vec = data_it->second.toByteVector();
  return decode_image({(unsigned char*)data_vec.data(), data_vec.size()});
}

struct Database {
  
  struct Track {
    
    std::string file_path;
    std::string properties[3];
  };
  
  static constexpr const char* properties_v[] = {
    "Title", "Artist", "Album"
  };
  
  std::vector<Track> tracks;
  std::set<std::string> artists; // We want artists ordered by default
  
  bool add_track_from_file(const std::string& path) {
    TagLib::FileRef f{path.c_str()};
    if (!f.tag())
      return false;
    auto tag = f.tag();
    auto Title = tag->title().to8Bit();
    auto Artist = tag->artist().to8Bit();
    auto Album = tag->album().to8Bit();
    if (Title == "")
      Title = ghc::filesystem::path(path).stem();
    tracks.push_back( {path, {Title, Artist, Album}} );
    try_add_artist(Artist);
    return true;
  }
  
  bool empty() const { return tracks.empty(); }
  auto num_tracks() const { return tracks.size(); }
  
  auto& track(int index) { return tracks[index]; }
  
  private : 
  
  void try_add_artist(const std::string& str) {
    if (str == "")
      return;
    artists.insert(str);
  }
};

template <>
struct table_model<std::vector<Database::Track>> {
  auto&& properties(ignore) { return Database::properties_v; }
  auto& cells(auto& self) { return self; }
  auto& cell_properties(Database::Track& t) { return t.properties; }
};
  
struct State : app_state {
  
  struct Table {
    
    static constexpr const char* properties_v[] = {
      "Title", "Artist", "Album"
    };
    
    auto&& properties () const {
      return properties_v;
    }
    
    unsigned num_properties() const { return 3; }
    
    struct cell {
      std::string file_path;
      std::string properties[3];
    };
    
    std::vector<cell> cells;
  };
  
  State() 
  {
  }
  
  bool set_play(bool Play) {
    if (Play)
      play();
    else
      pause();
    return is_playing;
  }
  
  void play() {
    if (database.empty())
      return;
    if (!current_track) {
      current_track = 0;
    }
    if (is_playing)
      pause();
    if (buffer_track_id != current_track)
    {
      auto id = *current_track;
      auto& path = database.track(id).file_path;
      auto buf = read_audio_file(path);
      if (!buf)
        return;
      player.buffer = *buf;
      player.head = 0;
      buffer_track_id = *current_track;
      auto cover = read_file_cover(path);
      if (cover) {
        current_cover = std::move(*cover);
        current_cover_updated = true;
      }
      player.done_reading = false;
    }
    
    player.start_audio_render();
    is_playing = true;
  }
  
  void pause() {
    player.stop_audio_render();
    is_playing = false;
  }
  
  void next_track() {
    current_track = *current_track + 1;
    if (*current_track > database.num_tracks())
      current_track = std::nullopt;
    else
      play();
  }
  
  void previous_track() {
    if (current_track && *current_track != 0)
      play_track(*current_track - 1);
  }
  
  void play_track(int id) {
    current_track = id;
    play();
  }
  
  void load_track(const std::string& path) {  
    if (database.add_track_from_file(path))    
      table_mutated = true;
  }
  
  std::string_view current_track_name() {
    if (current_track)
      return database.track(*current_track).properties[0];
    return "No track playing";
  }
  
  void check_done_reading() {
    if (player.done_reading) {
      next_track();
    }
  }
  
  auto&& artists() {
    return database.artists;
  }
  
  auto&& songs() {
    return database.tracks;
  }
  
  void set_current_artist(int index) {
    
  }
  
  void set_presentation(int index) {
    presentation_kind = index;
  }
  
  TrackPlayer player;
  std::optional<int> current_track;
  int buffer_track_id = -1;
  bool is_playing = false;
  
  Database database;
  unsigned presentation_kind = 0;
  bool table_mutated = false;
  
  image<rgba<unsigned char>> current_cover;
  bool current_cover_updated = false;
};

void paint_play_button(painter& p, bool flag, vec2f sz) {
  p.fill_style(colors::white);
  if (!flag)
    p.triangle({0, 0}, {0, sz.y}, {sz.x, sz.y / 2});
  else {
    auto rsz = vec2f{sz.x / 5, sz.y};
    p.rectangle({0, 0}, rsz);
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

auto artist_discography(State& state) {

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
  
  auto top_panel = hstack {
    hstack {
      graphic_trigger_button{&paint_transport_button<false>, &State::previous_track},
      graphic_toggle_button{&paint_play_button, state.is_playing, &State::set_play}.size({20, 20}),
      graphic_trigger_button{&paint_transport_button<true>, &State::next_track}
    },
    text{state.current_track_name()},
    slider{ [] (auto& s) -> auto& { return s.player.volume; } }
  }.interspace(30);
  
  bool update_cover = std::exchange(state.current_cover_updated, false);
  
  auto side_panel = list{{"Songs", "Artists", "Albums"}, false}
                     .on_select_cell( &State::set_presentation );
  
  bool update_table = std::exchange(state.table_mutated, false);
  
  auto songs_table = table{ state.database.tracks, update_table }
                      .on_cell_double_click( &State::play_track )
                      .on_file_drop( &State::load_track );
  
  auto center_view = either {
    state.presentation_kind,
    songs_table,
    list{ state.artists(), false }.on_select_cell(&State::set_current_artist), 
    //maybe{ state.current_artist, artist_discography(state) }
    //list{ state.albums(), false }.on_select_cell(&State::set_current_album)
  };
  
  return vstack{ top_panel,
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