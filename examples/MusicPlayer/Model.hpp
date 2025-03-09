#pragma once

#include "weave.hpp"

#include <taglib/tag.h>
#include <taglib/fileref.h>
#include <taglib/tpropertymap.h>

#include <ghc/filesystem.hpp>
#include <atomic>
#include <set>
#include <ranges>

using namespace weave;

struct TrackPlayer : weave::audio_renderer<TrackPlayer>
{
  void render_audio(weave::audio_output_stream& os) {
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
  
  std::atomic<float> volume = 1.f;
  weave::audio_buffer buffer;
  int head = -1;
  std::atomic<bool> done_reading = false;
};

static optional<weave::image<weave::rgba<unsigned char>>> read_file_cover(const std::string& path) {
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
  return weave::decode_image({(unsigned char*)data_vec.data(), data_vec.size()});
}

struct Database {
  
  using Self = Database;
  Database() = default;
  Database(const Database&) = delete;
  
  struct Track {
    
    auto& title() { return properties[0]; }
    auto& artist() { return properties[1]; }
    auto& album() { return properties[2]; }
    
    std::string file_path;
    std::string properties[3];
  };
  
  struct Playlist {
    std::string name;
    std::vector<int> tracks;
  };
  
  struct Album { 
    
    // Name and artist
    using Key = weave::tuple<std::string_view, std::string_view>;
    
    bool less(Key o) const {
      auto [name2, artist_name2] = o;
      if (name < name2)
        return true;
      if (name2 > name)
        return false;
      if (artist_name < artist_name2)  
        return true;
      return false;
    }
    
    bool operator<(const Album& o) const {  
      return less({o.name, o.artist_name});
    }
    
    bool operator<(tuple<std::string_view, std::string_view> o) const {
      return less(o);
    }
    
    bool operator==(const Album& o) const {
      return name == o.name && artist_name == o.artist_name;
    }
    
    bool operator==(Key k) const {
      return name == get<0>(k) && artist_name == get<1>(k);
    }
    
    bool operator!=(auto& o) const { return not ((*this) == o); }
    
    std::string name;
    std::string artist_name;
    weave::image<weave::rgb_u8> cover;
    std::vector<int> tracks;
  };
  
  static constexpr const char* properties_v[] = {
    "Title", "Artist", "Album"
  };
  
  std::vector<Track> tracks;
  std::set<std::string> artists; // We want artists ordered by default
  std::vector<Playlist> playlists_v;
  std::deque<Album> albums;
  
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
    try_add_album(tracks.back(), tracks.size() - 1);
    return true;
  }
  
  bool empty() const { return tracks.empty(); }
  auto num_tracks() const { return tracks.size(); }
  
  auto& track(int index) { return tracks[index]; }
  
  /// Accessor
  auto& playlists() const { 
    return playlists_v; 
  }
  
  void add_playlist() {
    playlists_v.push_back({"Playlist1"});
  }
  
  auto& playlist(int id) {
    return playlists()[id];
  }
  
  struct AlbumTracks {
    
    auto tracks() {
      return std::views::transform(self.album(album_id).tracks, [s = &self] (auto id) -> auto& { return s->track(id); });
    }
    
    Self& self;
    int album_id;
  };
  
  AlbumTracks album_tracks(int id) {
    return {*this, id};
  }
  
  Album& album(int id) {
    return albums[id];
  }
  
  void add_to_playlist(int playlist_id, int track_id) {
    playlists_v[playlist_id].tracks.push_back(track_id);
  }
  
  private : 
  
  void try_add_artist(const std::string& str) {
    if (str == "")
      return;
    artists.insert(str);
  }
  
  void try_add_album(Track& track, int track_id) {
    auto key = weave::tuple{std::string_view{track.album()}, std::string_view{track.artist()}};
    auto it = std::lower_bound(albums.begin(), albums.end(), key);
    if (it == albums.end() || *it != key) {
      it = albums.insert(it, Album{std::string(get<0>(key)), std::string(get<1>(key))});
    }
    if (it->cover.empty()) {
      auto c = read_file_cover(track.file_path);
      if (c)
        it->cover = c->to<weave::rgb_u8>();
    }
    it->tracks.push_back(track_id);
  }
};

template <>
struct weave::table_model<std::vector<Database::Track>> {
  auto&& properties(ignore) { return Database::properties_v; }
  auto& cells(auto& self) { return self; }
  auto& cell_properties(Database::Track& t) { return t.properties; }
};

template <>
struct weave::table_model<Database::AlbumTracks> {
  auto&& properties(ignore) { return Database::properties_v; }
  auto cells(auto& obj) { return obj.tracks(); }
  auto& cell_properties(Database::Track& t) { return t.properties; }
};

namespace fs = ghc::filesystem; 

inline bool is_audio_file(fs::path path) {
  auto ext = path.extension().string();
  return ext == ".wav" || ext == ".WAV" || ext == ".mp3" || ext == ".aiff" || ext == ".aif"
        || ext == ".flac" || ext == ".FLAC";
}

struct State : weave::app_state {
  
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
  
  auto&& artists() { return database.artists; }
  auto&& songs() { return database.tracks; }
  auto& playlists() const { return database.playlists(); }
  
  void load_directory(ghc::filesystem::path p) {
    for (auto&& f : fs::recursive_directory_iterator(p))
      if (!is_directory(f) && is_audio_file(f.path()))
        load_track(f.path().string());
  }
  
  void add_playlist() { database.add_playlist(); }
  
  void add_to_playlist(int playlist_id, int track_id) {
    database.add_to_playlist(playlist_id, track_id);
  }
  
  TrackPlayer player;
  std::optional<int> current_track;
  int buffer_track_id = -1;
  bool is_playing = false;
  
  Database database;
  bool table_mutated = false;
  
  weave::image<rgba<unsigned char>> current_cover;
  bool current_cover_updated = false;
};