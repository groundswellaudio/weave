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

audio_buffer load_audio_file(const std::string& path) {
  drwav wav;
  if (!drwav_init_file_with_metadata(&wav, path.c_str(), 0, NULL))
    return {};
  auto dtor = on_exit( [&] () { drwav_uninit(&wav); } );
  drwav_seek_to_pcm_frame(&wav, 0);
  std::vector<float> vec;
  vec.resize( wav.channels * wav.totalPCMFrameCount );
  drwav_read_pcm_frames_f32(&wav, wav.totalPCMFrameCount, vec.data());
  return {vec, wav.channels};
}