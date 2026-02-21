#pragma once

#include <type_traits>

#define WEAVE_FWD(X) static_cast<decltype(X)&&>(X)

#define WEAVE_MOVE(X) static_cast<std::remove_reference_t<decltype(X)>&&>(X)