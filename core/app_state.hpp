#pragma once

struct app_state {
  bool read_scope() const { return true; }
  bool write_scope() const { return true; }
};
