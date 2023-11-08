#pragma once
#include "VkBootstrap.h"

namespace core {

class Base {
public:
  Base(vkb::DispatchTable &disp) : disp{disp} {}

protected:
  vkb::DispatchTable &disp;
};

} // namespace core