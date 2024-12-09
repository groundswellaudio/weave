#pragma once

#include "spiral.hpp"

#include <taglib/tag.h>
#include <taglib/fileref.h>
#include <taglib/tpropertymap.h>

#include <ghc/filesystem.hpp>
#include <atomic>

struct TrackPlayer : audio_renderer<TrackPlayer>
{
  void render_audio(audio_output_stream& os) {
    for (auto s : os)
    {
      s[0] = buffer[head] * volume;
      s[1] = buffer[head + 1] * volume;
      head += 2;
    }
  }
  
  std::atomic<float> volume;
  audio_buffer buffer;
  int head = 0;
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
  
  void set_play(bool Play) {
    if (Play)
      play();
    else
      pause();
  }
  
  void play() {
    if (!current_track)
      current_track = 0;
    if (is_playing)
      pause();
    if (buffer_track_id != current_track)
    {
      auto id = *current_track;
      auto& path = table.cells[id].file_path;
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
    }
    
    player.start_audio_render();
    is_playing = true;
  }
  
  void pause() {
    player.stop_audio_render();
    is_playing = false;
  }
  
  void play_track(int id) {
    current_track = id;
    play();
  }
  
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
  
  void load_track(const std::string& path) {
    TagLib::FileRef f{path.c_str()};
    if (!f.tag())
      return;
    auto tag = f.tag();
    auto Title = tag->title().to8Bit();
    auto Artist = tag->artist().to8Bit();
    auto Album = tag->album().to8Bit();
    table.cells.push_back( {path, {Title, Artist, Album}} );
    table_mutated = true;
  }
  
  std::string_view current_track_name() {
    if (current_track)
      return table.cells[*current_track].properties[0];
    return "No track playing";
  }
  
  TrackPlayer player;
  
  audio_buffer track_data;
  int buffer_track_id = -1;
  
  std::string PlaylistName;
  Table table;
  
  std::optional<int> current_track;
  bool is_playing = false;
  
  bool table_mutated = false;
  
  image<rgba<unsigned char>> current_cover;
  bool current_cover_updated = false;
};

auto make_view(State& state)
{
  using namespace views;
  
  bool update_table = std::exchange(state.table_mutated, false);
  
  /* 
  auto on_file_drop = [] (event_context_t<void>& ec, const std::string& path) {
    if (is_directory(ghc::path{path})
      std::cout << "got directory" << std::endl;
    ec.state<State>().load_track()
  }; */ 
  
  auto tablev = table { state.table, update_table }
                 .on_cell_double_click( &State::play_track )
                 .on_file_drop( &State::load_track );
  
  auto top_panel = hstack {
    toggle_button{ "Play", readwrite(&State::is_playing, &State::set_play) },
    text{state.current_track_name()},
    slider{ [] (auto& s) -> auto& { return s.player.volume; } }
  }.interspace(30);
  
  bool update_cover = std::exchange(state.current_cover_updated, false);
  
  return vstack{ top_panel,
                 hstack{tablev, 
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