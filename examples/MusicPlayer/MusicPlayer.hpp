#pragma once

#include "spiral.hpp"

#include <taglib/tag.h>
#include <taglib/fileref.h>
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
      auto buf = read_audio_file(table.cells[*current_track].file_path);
      if (!buf)
        return;
      player.buffer = *buf;
      player.head = 0;
      buffer_track_id = *current_track;
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
  
  void load_track(const std::string& path) {
    //auto buf = load_audio_file(path);
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
  
  return vstack{ top_panel,
                 tablev,
               }.align_center().margin({10, 10});
}

inline void run_app() {
  State state;
  auto app = make_app(state, &make_view);
  app.run(state);
}