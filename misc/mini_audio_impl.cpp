#include "audio.hpp"
#include "util/util.hpp"
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#define DR_MP3_IMPLEMENTATION
#define DR_FLAC_IMPLEMENTATION
#define DR_WAV_IMPLEMENTATION

#include "dr_mp3.h"
#include "dr_flac.h"
#include "dr_wav.h"

optional<audio_buffer> weave::read_audio_file(const std::string& path) 
{
  if (path.ends_with(".mp3"))
  {
    drmp3 mp3;
    if (!drmp3_init_file(&mp3, path.c_str(), nullptr))
      return {};
    auto dtor = on_exit( [&] () { drmp3_uninit(&mp3); } );
    drmp3_seek_to_pcm_frame(&mp3, 0);
    
    std::vector<float> vec;
    float Temp[4096];
    while(true)
    {
      auto framesJustRead = drmp3_read_pcm_frames_f32(&mp3, std::size(Temp) / mp3.channels, Temp);
      if (framesJustRead == 0)
        break;
      vec.append_range(Temp);
    }

    return audio_buffer{vec, (int)mp3.channels};
  }
  else if (path.ends_with(".flac"))
  {
    return {};
  }
  else // Default ot wav if all else fails 
  {
    drwav wav;
    if (!drwav_init_file_with_metadata(&wav, path.c_str(), 0, nullptr))
      return {};
    auto dtor = on_exit( [&] () { drwav_uninit(&wav); } );
    drwav_seek_to_pcm_frame(&wav, 0);
    std::vector<float> vec;
    vec.resize( wav.channels * wav.totalPCMFrameCount );
    drwav_read_pcm_frames_f32(&wav, wav.totalPCMFrameCount, vec.data());
    return audio_buffer{vec, (int)wav.channels};
  }
}