#pragma once

namespace weave {

struct app_state {
  void apply_write(this auto& self, auto&& fn) {
    fn(self);
  }
  decltype(auto) apply_read(this auto& self, auto&& fn) {
    return (fn(self));
  }
};

}