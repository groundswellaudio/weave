#pragma once
#include <SDL.h>

#include "deps/miniaudio/miniaudio.h"

struct audio_buffer_format {
  int sample_rate = 44100;
  int buffer_size = 512;
  unsigned char num_channels = 2;
};

struct audio_output_stream
{
	struct sample
	{
		auto& operator[](int x) { return ptr[x]; }
		float* ptr;
	};
	
	struct sample_iterator_sentinel
	{
		float* end_ptr;
	};
	
	struct sample_iterator
	{
		auto& operator++()        { ptr += n_channels; return *this; }
		sample operator*() const  { return {ptr};                    }
		
		bool operator!=(sample_iterator_sentinel s) const {
			return ptr != s.end_ptr;
		}
		
		float* ptr;
		const int n_channels;
	};
	
	auto begin() { return sample_iterator{ptr, n_channels};                            }
	auto end()   { return sample_iterator_sentinel{ptr + n_channels * n_samples};      }
	
	float* ptr;
	int n_channels;
	int n_samples;
};

template <class T>
struct audio_renderer 
{
  using Self    = audio_renderer<T>;

  void start_audio_render(audio_buffer_format fmt = {})
  {
    start_audio_device(fmt, 1);
  }

  void start_audio_input(audio_buffer_format fmt)
  {
    start_audio_device(fmt, 1, true);
  }

  void stop_audio_render()
  {
    ma_device_stop(&device);
  }

  ~audio_renderer() {
    stop_audio_render();
    ma_device_uninit(&device);
  }

  private :
	
  void start_audio_device(audio_buffer_format fmt, int id, bool is_input = false)
  {
    auto callback = +[] (ma_device* device, void* output, const void* input, unsigned nFrames)
    {
      auto& self = *reinterpret_cast<T*>(device->pUserData);
      audio_output_stream ostrm
      {
        reinterpret_cast<float*>(output),
        (int) device->playback.channels,
        (int) nFrames
      };
    
      self.render_audio(ostrm);
    };
    
    ma_device_config config   = ma_device_config_init(ma_device_type_playback);
    config.playback.format    = ma_format_f32;   // Set to ma_format_unknown to use the device's native format.
    config.playback.channels  = fmt.num_channels;               // Set to 0 to use the device's native channel count.
    config.sampleRate         = 44100;           // Set to 0 to use the device's native sample rate.
    config.dataCallback       = callback;   // This function will be called when miniaudio needs more data.
    config.periodSizeInFrames = fmt.buffer_size;
    config.pUserData          = static_cast<T*>(this);   // Can be accessed from the device object (device.pUserData).
    
    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS)
        return;  // Failed to initialize the device.
    
    ma_device_start(&device);
  }

  ma_device device;
  audio_buffer_format current_format;
};