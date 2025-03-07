#pragma once

#include <miniaudio.h>
#include <string>
#include <string_view>
#include <vector>
#include <cassert>
#include <util/optional.hpp>

namespace weave {

struct audio_buffer_format {
  int sample_rate = 44100;
  int buffer_size = 512;
  unsigned char num_channels = 2;
};

struct audio_buffer : std::vector<float> {
  int num_channels;
};

optional<audio_buffer> read_audio_file(const std::string& path);

struct audio_device_handle {
  
  std::string_view name() const { return device->name; }
  
  ma_device_info* device;
};

struct audio_devices_range {
  
  struct iterator {
    using value_type = audio_device_handle;
    auto& operator++() { ++ptr; return *this; }
    auto operator++(int) { auto res{*this}; ++*this; return res; }
    value_type operator*() const { return audio_device_handle{ptr}; }
    bool operator==(iterator o) const { return ptr == o.ptr; }
    bool operator!=(iterator o) const { return ptr != o.ptr; }
    auto operator-(iterator o) const { return ptr - o.ptr; }
    
    ma_device_info* ptr; 
  };
  
  auto begin() { return iterator{devices}; }
  auto end() { return iterator{devices + count}; }
  auto operator[](int index) { return audio_device_handle{devices + index}; }
  
  ma_device_info* devices;
  unsigned count;
};

static_assert( std::input_iterator<audio_devices_range::iterator> );

namespace impl {
  inline ma_context* audio_context() {
    static ma_context ctx = [] () {
      ma_context context;
      auto InitRes = ma_context_init(NULL, 0, NULL, &context);
      assert( InitRes == MA_SUCCESS );
      return context;
    } ();
    return &ctx;
  }
}

inline audio_devices_range audio_input_devices() {
  auto ctx = impl::audio_context();
  ma_device_info* pCaptureInfos;
  ma_uint32 captureCount;
  ma_context_get_devices(ctx, nullptr, nullptr, &pCaptureInfos, &captureCount);
  return {pCaptureInfos, captureCount};
}

inline audio_devices_range audio_output_devices() {
  auto ctx = impl::audio_context();
  ma_device_info* devicesInfos;
  ma_uint32 count;
  ma_context_get_devices(ctx, &devicesInfos, &count, nullptr, nullptr);
  return {devicesInfos, count};
}

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
	
	auto sample_size() const { return n_samples; }
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

  audio_renderer() {
    device.pContext = nullptr;
  }
  
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
  
  void set_audio_device(int id) {
    stop_audio_render();
    start_audio_device(current_format, id);
  }
  
  int current_device_index() const {
    return device_index;
  }

  ~audio_renderer() {
    if (device.pContext != nullptr) {
      stop_audio_render();
      ma_device_uninit(&device);
    }
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
    
    device_index = id;
    
    ma_device_config config   = ma_device_config_init(ma_device_type_playback);
    config.playback.pDeviceID = &audio_output_devices()[id].device->id;
    config.playback.format    = ma_format_f32;   // Set to ma_format_unknown to use the device's native format.
    config.playback.channels  = fmt.num_channels;  // Set to 0 to use the device's native channel count.
    config.sampleRate         = 44100;           // Set to 0 to use the device's native sample rate.
    config.dataCallback       = callback;   // This function will be called when miniaudio needs more data.
    config.periodSizeInFrames = fmt.buffer_size;
    config.pUserData          = static_cast<T*>(this);   // Can be accessed from the device object (device.pUserData).
    
    if (ma_device_init(impl::audio_context(), &config, &device) != MA_SUCCESS)
        return;  // Failed to initialize the device.
    
    ma_device_start(&device);
  }

  ma_device device;
  audio_buffer_format current_format;
  int device_index = 0;
};

} // weave