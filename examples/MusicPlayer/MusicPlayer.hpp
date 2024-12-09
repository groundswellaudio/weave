#pragma once

#include "spiral.hpp"

#include <taglib/tag.h>
#include <taglib/fileref.h>
#include <ghc/filesystem.hpp>

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
      std::string properties[3];
    };
    
    std::vector<cell> cells;
  };
  
  State() 
  {
    table.cells.push_back({"ksian boktet", "jupiter", "gloom"});
    table.cells.push_back({"autechre", "bleep bloop", "scrunch"});
    table.cells.push_back({"alicia keys", "unthinkable", "really good"});
  }
  
  void play_track(int id) {
    current_track = id;
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
    table.cells.push_back( {Title, Artist, Album} );
    table_mutated = true;
  }
  
  std::string_view current_track_name() {
    if (current_track)
      return table.cells[*current_track].properties[1];
    return "";
  }
  
  audio_buffer track_data;
  
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
  
  return vstack{ maybe{ state.current_track, text{state.current_track_name()} },
                 table { state.table, update_table }
                  .on_cell_double_click( &State::play_track )
                  .on_file_drop( &State::load_track ),
                 text_field{ &State::PlaylistName }
               }.align_center();
}

inline void run_app() {
  State state;
  auto app = make_app(state, &make_view);
  app.run(state);
}