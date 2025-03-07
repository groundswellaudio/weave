#pragma once

#include "spiral.hpp"

#include <ranges>
#include <random>
#include <future>

template <class T>
struct padded_image {
  
  template <class P>
  using pixel_template = image<T>::template pixel_template<P>;
  
  using pixel_type = T;
  
  bool empty() const { return img.empty(); }
  
  constexpr auto shape() const {
    return img.shape() - margin * 2;
  }
  
  void reshape(vec2i new_shape) {
    img.reshape(new_shape + margin * 2);
  }
  
  constexpr auto& operator()(this auto& self, vec2i idx) {
    return self.img(idx + self.margin);
  }
  
  template <class Pixel>
  image<Pixel> to() const {
    image<Pixel> res;
    res.reshape(shape());
    for (int x0 : iota(shape()[0]))
      for (int x1 : iota(shape()[1]))
        res(x0, x1) = static_cast<Pixel>((*this)(x0, x1));
    return res;
  }
  
  template <class I>
  constexpr bool operator==(const I& o) const {
    if (shape() != o.shape())
      return false;
    for (int x0 = 0; x0 < shape()[0]; ++x0)
      for (int x1 = 0; x1 < shape()[1]; ++x1)
        if ((*this)(vec2i{x0, x1}) != o(vec2i{x0, x1}))
          return false;
    return true; 
  }
  
  constexpr auto& operator()(this auto& self, int y, int x) {
    return self({y, x});
  }
  
  void set_padding(vec2i pad) { margin = pad; }
  
  image<T> img; 
  vec2i margin;
};

template <class T>
auto wrap(T x, T x_min, T x_max){
  if (x < x_min)
    return x_max - (x_min - x) % (x_max - x_min);
  else if (x >= x_max)
    return x_min + (x - x_min) % (x_max - x_min);
  else
    return x;
}

template <class I>
auto patch_distance(const I& a, vec2<int> idxa, const I& b, vec2<int> idxb, int patch_hs) 
{
  assert( idxa.x < a.shape().x );
  assert( idxa.y < a.shape().y );
  assert( idxb.x < b.shape().x );
  assert( idxb.y < b.shape().y );
  
  decltype(distance_squared(a(0, 0), b(0, 0))) sum = 0;
  for (auto y : iota(-patch_hs, patch_hs + 1))
  {
    for (auto x : iota(-patch_hs, patch_hs + 1))
    {
      sum += distance_squared( a(idxa + vec2i{y, x}), b(idxb + vec2i{y, x}) );
    }
  }
  return sum;
}

template <class I>
auto mean(const I& i) {
  using RT = typename I::template pixel_template<float>;
  RT res {0};
  for (auto y : iota(i.shape()[0]))
  {
    RT acc{0};
    for (auto x : iota(i.shape()[1]))
      acc += i(y, x);
    acc /= (float) i.shape()[1];
    res += acc;
  }
  res /= (float) i.shape()[0];
  return res;
}

template <class I, class M>
auto variance(const I& i, M mean) {
  M res {0};
  for (auto y : iota(i.shape()[0]))
  {
    M acc{0};
    for (auto x : iota(i.shape()[1])) {
      M delta = i(y, x) - mean;
      delta *= delta;
      acc += delta;
    }
    acc /= i.shape()[1];
    res += acc;
  }
  res /= i.shape()[0];
  return res;
}

template <class I, class Fn>
void for_each_pixel(I& image, Fn fn) {
  for (auto y : iota(image.shape()[0])) 
    for (auto x : iota(image.shape()[1]))
      fn( image(y, x), vec2i{y, x} );
}

template <class I>
void match_distribution(const I& examplar, I& generated) {
  auto mg = mean(generated);
  auto dg = sqrt( variance(generated, mg) );
  auto me = mean(examplar);
  auto de = sqrt( variance(examplar, me) );
  for_each_pixel( generated, [&] (auto& pix, ignore) {
    pix = ((pix - mg) * de) / dg + me;
    for (int k = 0; k < I::pixel_type::channels; ++k)
      pix[k] = std::clamp(pix[k], typename I::pixel_type::scalar{0}, I::pixel_type::norm());
  });
}

using search_map = image<tuple<vec2i, float>>;

template <class I>
void patch_match_search(const I& examplar, I& generated, search_map& Map, int patch_hs)
{ 
  assert( Map.shape() == generated.shape() );
  
  std::random_device rd;
  std::default_random_engine gen(rd());
  std::uniform_int_distribution<int> rand{-10000, 10000};
  
  for_each_pixel( generated, [&] (auto& pix, vec2i idx) {
    auto y = idx[0];
    auto x = idx[1];
    
    float min_dist = Map(idx).m1;
    vec2i origin = Map(idx).m0;
    
    int ratio = 1;
    int max_size = std::min(examplar.shape()[0], examplar.shape()[1]);
    
    for(; max_size / ratio > 1; ratio *= 2)
    {
      auto rx = rand(gen) % max_size;
      auto ry = rand(gen) % max_size;
      auto nx = origin.x + rx;
      auto ny = origin.y + ry;
      nx = wrap(nx, 0, examplar.shape().x - 1);
      ny = wrap(ny, 0, examplar.shape().y - 1);
      
      auto new_index = vec2i{nx, ny};
      
      auto patch_dist = patch_distance(examplar, new_index, generated, idx, patch_hs);
      if (patch_dist < min_dist) {
        min_dist = patch_dist;
        origin = new_index;
      }
    }
    
    Map(idx) = tuple{origin, min_dist};
  });
}

template <bool B, class R>
constexpr auto maybe_reverse(R range) {
  if constexpr (B)
    return std::ranges::reverse_view(range);
  else
    return range;
}

template <class Range>
struct drop_first_n {
    
  constexpr auto begin(this auto& self) { 
    auto it = self.range.begin(); 
    int k = self.skip; 
    while(k--) {
      ++it;
    }
    return it;
  }
  
  constexpr auto end(this auto& self) { return self.range.end(); }

  Range range;
  int skip = 1;
};

template <class Range>
constexpr auto drop_first(Range&& range, int skip) {
  return drop_first_n<Range>{(Range&&)range, skip};
}

template <int Sign, class I>
void patch_match_propagation(I& examplar, I& generated, search_map& Map, int patch_hs) 
{
  std::random_device rd;
  std::default_random_engine gen(rd());
  std::uniform_int_distribution distrib{0, 100};
  
  static_assert( Sign == -1 || Sign == +1 );
  auto yrange = maybe_reverse<Sign == -1>(iota(0, Map.shape()[0]));
  auto xrange = maybe_reverse<Sign == -1>(iota(0, Map.shape()[1]));
  
  for (auto y : drop_first(yrange, 1))
  {
    for (auto x : drop_first(xrange, 1))
    {
      /* 
      // skip propagation for 30 percent of pixels
      if (distrib(gen) > 70)
        continue; */ 
      
      auto try_propagate = [&] (int dy, int dx) {
        
        auto new_idx = Map(y - dy, x - dx).m0 + vec2i{dy, dx};
        // Don't propagate if we reached the end/beginning of examplar
        if constexpr (Sign > 0) {
          if (new_idx[0] == examplar.shape()[0]
             || new_idx[1] == examplar.shape()[1])
            return;
        }
        else {
          if (new_idx[0] == -1 || new_idx[1] == -1)
            return;
        }
        
        auto patch_dist = patch_distance(examplar, new_idx, generated, vec2i{y, x}, patch_hs);
        if (patch_dist < Map(y, x).m1) {
          Map(y, x).m1 = patch_dist;
          Map(y, x).m0 = new_idx;
        }
      };
      
      try_propagate(0, Sign);
      try_propagate(Sign, 0);
    }
  }
}

template <class R1, class R2>
void for_point_in(R1&& a, R2&& b, auto Fn) {
  using scalar = decltype(*a.begin());
  for (auto x0 : a)
    for (auto x1 : b)
      Fn(vec2<scalar>{x0, x1});
}


template <class I>
void patch_match_synthesize(I& source, I& generated, search_map& map)
{
  for_each_pixel( generated, [&] (auto& pix, vec2i idx) {
    pix = source(map(idx).m0);
  });
  
  /* 
  for_point_in( iota(1, generated.shape()[0] - 1), iota(1, generated.shape()[1] - 1), 
                [&] (vec2i idx) {
                  auto acc = [&] (vec2i delta) {
                    generated(idx) += source(map(idx + delta).m0 - delta);
                  };
                  generated(idx) += generated(idx);
                  acc({1, 1});
                  acc({1, 0});
                  acc({1, -1});
                  acc({0, 1});
                  acc({0, -1});
                  acc({-1, 1});
                  acc({-1, 0});
                  acc({-1, -1});
                  generated(idx) /= 10;
                }); */ 
}

/// Patch match resynthesis with completeness constraints
template <class I>
void patch_match_synthesize(const I& source, I& generated, const search_map& map, const search_map& invmap)
{
  image<monochrome<float>> norm {generated.shape()};
  for (auto& n : norm)
    n = 1;
  
  // resampling
  for_each_pixel( generated, [&] (auto& pix, vec2i idx) {
    pix = source(map(idx).m0);
  });
  
  // completeness, combine the two maps by finding the pick with the least distance
  for_each_pixel( source, [&] (auto& pix, vec2i idx) {
    auto mapped_idx = invmap(idx).m0;
    generated(mapped_idx) += pix;
    ++norm(mapped_idx).value;
  });
  
  // normalize
  for_each_pixel( generated, [&] (auto& pix, vec2i idx) {
    pix /= norm(idx).value;
  });
}

template <class I>
void patch_match_update_distance(const I& source, const I& generated, search_map& map, int patch_hs)
{
  for_each_pixel( map, [&] (auto& elem, vec2i idx) {
    elem.m1 = patch_distance(source, elem.m0, generated, idx, patch_hs);
  });
}

consteval type distrib_type(type scalar) {
  auto args = template_arguments{scalar};
  if (is_floating_point(scalar))
    return instantiate(^std::uniform_real_distribution, args);
  else
    return instantiate(^std::uniform_int_distribution, args);
}

template <class I>
void fill_with_noise(I& img) {
  std::random_device rd;
  std::default_random_engine gen(rd());
  using scalar = typename I::pixel_type::scalar;
  using distrib_t = %distrib_type(^scalar);
  distrib_t
    distrib(0, I::pixel_type::norm());
  for_each_pixel( img, [&] (auto& pix, ignore) {
    for (auto k : iota(I::pixel_type::channels))
      pix[k] = distrib(gen);
  });
}

void dump_search_map(const search_map& m) {
  for (auto& e : m)
    std::cout << e.m0 << " ";
}

void reset_search_map(search_map& map, vec2i source_bounds) {
  std::random_device rd;
  std::default_random_engine gen(rd());
  std::uniform_int_distribution<int> gen0 (0, source_bounds[0]);
  std::uniform_int_distribution<int> gen1 (0, source_bounds[1]);
  for (auto& entry : map) {
    entry.m0 = vec2i{gen0(gen), gen1(gen)};
    entry.m1 = 1000000000000.f;
  }
}

using histogram_t = std::vector<float>;

template <class I>
void histogram(const I& img, int channel, histogram_t& result, int NumBins = 256) {
  for_each_pixel(img, [&] (auto& pix, ignore) {
    auto val = pix[channel];
    auto Bin = (val * NumBins) / I::pixel_type::norm();
    ++result[Bin];
  });
}

template <class I>
void cdf(const I& img, int channel, histogram_t& result, int NumBins = 256) {
  result.resize(NumBins + 1);
  histogram(img, channel, result, NumBins);
  histogram_t::value_type acc {0};
  for (auto& Bin : result) {
    acc += Bin;
    Bin = acc;
  }
  for (auto& Bin : result)
    Bin /= acc;
}

template <class I>
void histogram_matching(const I& source, I& img, int numBins = 256) {
  constexpr auto channels = I::pixel_type::channels;
  std::array<histogram_t, channels> cdf_source, cdf_img, lookuptable;
  for (int k : iota(channels))
    cdf(source, k, cdf_source[k], numBins);
  for (int k : iota(channels))
    cdf(img, k, cdf_img[k], numBins);
  for (int k : iota(channels)) {
    lookuptable[k].resize(numBins + 1);
    for (int b : iota(numBins)) {
      auto p = cdf_img[k][b];
      auto it = std::lower_bound(cdf_source[k].begin(), cdf_source[k].end(), p);
      lookuptable[k][b] = (I::pixel_type::norm() * (it - cdf_source[k].begin())) / cdf_source[k].size();
    }
  }
  for_each_pixel(img, [&] (auto& pix, ignore) {
    for (int k : iota(channels)) {
      pix[k] = lookuptable[k][(pix[k] * numBins) / I::pixel_type::norm()];
    }
  });
}

struct PatchMatchMap {
  
  template <class I>
  void search(const I& source, const I& img, int patch_hs) {
    patch_match_search(source, img, map, patch_hs);
  }
  
  template <class I>
  void propagate(const I& source, I& img, int patch_hs) {
    if (!flip_propagation)
      patch_match_propagation<1>(source, img, map, patch_hs);
    else
      patch_match_propagation<-1>(source, img, map, patch_hs);
    flip_propagation = !flip_propagation; 
  }
  
  search_map map;
  bool flip_propagation = false;
};

template <class I, class V>
void add_noise(I& img, V amplitude) {
  std::random_device rd;
  std::default_random_engine gen(rd());
  using scalar = typename I::pixel_type::scalar;
  using distrib_t = %distrib_type(^scalar);
  distrib_t
    distrib(amplitude, amplitude);
  for_each_pixel( img, [&] (auto& pix, ignore) {
    for (auto k : iota(I::pixel_type::channels))
      pix[k] += distrib(gen);
  });
}

struct TextureSynthesis : app_state {
  
  static constexpr auto max_gen_sz = vec2i{1080, 1920};
  
  TextureSynthesis() 
  : generated{vec2i{600, 800}}
  {
    generated.set_padding({20, 20});
    map.reshape(generated.shape());
    fill_with_noise(generated);
  }
  
  void load_image() {
    auto path = open_file_dialog();
    if (!path)
      return;
    auto res = load_image_from_file(*path);
    if (!res)
      return;
    //display = res->to<rgba<unsigned char>>();
    examplar = padded_image{res->to<rgb<float>>(), {15, 15}};
    invmap.reshape(examplar.shape());
    reset_search_map(map, examplar.shape());
    reset_search_map(invmap, generated.shape());
    
    refresh_examplar = true;
  }
  
  void save_image() {
    auto path = save_file_dialog();
    if (!path)
      return;
    auto saved = generated.to<rgba<unsigned char>>();
    auto success = save_image_to_file(*path, saved);
    assert( success );
  }
  
  void set_output_shape(vec2i new_shape) {
    generated.reshape(new_shape);
    map.reshape(new_shape);
    reset_search_map(map, examplar.shape());
  }
  
  void v1(std::function<void()> on_done) {
    auto fn = [this, on_done] {
      patch_match_search(examplar, generated, map, patch_hs);
      if (!flip_propagation)
        patch_match_propagation<1>(examplar, generated, map, patch_hs);
      else
        patch_match_propagation<-1>(examplar, generated, map, patch_hs);
      flip_propagation = !flip_propagation; 
      patch_match_synthesize(examplar, generated, map);
      histogram_matching(examplar, generated);
      patch_match_update_distance(examplar, generated, map, patch_hs);
      refresh_display = true;
      on_done();
      progress = -1;
    };
    progress = 0;
    synth_task = std::async(fn);
  }
  
  void inject_noise() {
    add_noise(generated, 0.1);
    patch_match_update_distance(examplar, generated, map, patch_hs);
    refresh_display = true;
  }
  
  void v2() {
    auto fn = [this] {
      patch_match_search(examplar, generated, map, patch_hs);
      patch_match_search(generated, examplar, invmap, patch_hs);
      if (!flip_propagation) {
        patch_match_propagation<1>(examplar, generated, map, patch_hs);
        patch_match_propagation<1>(generated, examplar, invmap, patch_hs);
      }
      else {
        patch_match_propagation<-1>(examplar, generated, map, patch_hs);
        patch_match_propagation<-1>(generated, examplar, invmap, patch_hs);
      }
      flip_propagation = !flip_propagation; 
      patch_match_synthesize(examplar, generated, map, invmap);
      patch_match_update_distance(examplar, generated, map, patch_hs);
      patch_match_update_distance(generated, examplar, invmap, patch_hs);
      progress = -1;
      refresh_display = true;
    };
    progress = 0;
    synth_task = std::async(fn);
  }
  
  bool is_working() const {
    return progress >= 0.f;
  }
  
  padded_image<rgb<float>> generated;
  padded_image<rgb<float>> examplar;
  search_map map, invmap;
  bool flip_propagation = true;
  
  int patch_hs = 8;
  
  std::atomic<float> progress = -1;
  std::future<void> synth_task;
  std::atomic<bool> refresh_display = false;
  bool refresh_examplar = false;
};

auto make_view(TextureSynthesis& state)
{
  using State = TextureSynthesis;
  
  auto img_size = min(state.examplar.shape(), {1600, 800});
  auto ImgPadding = 10;
  
  using namespace views;
  
  bool refresh_examplar = std::exchange(state.refresh_examplar, false);
  bool refresh_display = state.refresh_display.exchange(false);
  
  constexpr auto size_dial = [] (int k) {
    return numeric_field {
      readwrite( [k] (auto& s) { return s.generated.shape()[k]; }, 
                 [k] (auto& s, int val) { auto new_shape = s.generated.shape(); 
                                          new_shape[k] = val;
                                          s.set_output_shape(new_shape);
                                        }
               )
    }.range(100, 5000);
  };
  
  return vstack {
    text{ "Texture Synthesis from examplar"},
    trigger_button { "Load texture", &State::load_image }
    .disable_if(state.is_working()),
    trigger_button{ "Synthesize", [] (event_context& ec) { 
      ec.state<State>().v1(ec.lift_rebuild_request());
    }}
    .disable_if(state.is_working() || state.examplar.empty()),
    trigger_button{ "Save output", &TextureSynthesis::save_image }
    .disable_if(state.is_working()),
    trigger_button { "Inject noise", &TextureSynthesis::inject_noise }
    .disable_if(state.is_working()), 
    hstack {
      text( "Generated size" ),
      size_dial(1), 
      size_dial(0), 
      text( "Patch half-size " ), 
      numeric_field{ &State::patch_hs }.range(3, 20), 
    },
    views::image{ state.examplar, refresh_examplar }.fit({300, 300}), 
    hstack { 
      views::image{ state.generated, refresh_display }.fit({600, 600}),
      views::image{ state.map, refresh_display, [sz = state.examplar.shape()] (auto& elem) {
        return rgb<float>{elem.m0.x / (float) sz.x, elem.m0.y / (float) sz.y, 0}; 
      }}.fit({600, 600})
    },
    /*hstack {
      // views::image { state.display },
      
      views::image{ state.examplar, refresh_examplar }, 
      views::image{ state.map, refresh_display, [sz = state.examplar.shape()] (auto& elem) {
        return rgb<float>{elem.m0.x / (float) sz.x, elem.m0.y / (float) sz.y, 0}; 
      }}, 
    },*/
    progress_bar { state.progress }
  };
}

void test_image() {
  image<vec2i> img {
    vec2i{720, 725}
  };
  
  for_each_pixel( img, [] (auto& pix, vec2i idx) {
    pix = idx;
  });
  
  vec2i margin = {17, 17};
  
  padded_image<vec2i> img2 { img, margin };
  
  for_each_pixel( img2, [&] (auto& p2, vec2i idx) {
    assert( p2 == idx + margin );
  });
  
  auto img3 = img;
  img3.reshape(img3.shape() - margin * 2);
  for_each_pixel( img3, [&] (auto& pix, vec2i idx) {
    pix = img(idx + margin);
  });
  
  assert( img2 == img3 );
}

inline void run_app() {
  test_image();
  TextureSynthesis state;
  window_properties prop;
  prop.name = "TextureSynthesis";
  auto app = make_app(state, &make_view, prop);
  app.run(state);
}