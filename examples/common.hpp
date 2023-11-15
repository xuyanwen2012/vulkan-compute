#pragma once

#include <algorithm>
#include <iostream>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "spdlog/spdlog.h"

[[nodiscard]] inline std::ostream &operator<<(std::ostream &os,
                                              const glm::vec4 &v) {
  os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
  return os;
}

inline void setup_log_level(std::string log_level) {
#if defined(NDEBUG)
  spdlog::set_level(spdlog::level::off);
  return;
#endif
  std::ranges::transform(log_level, log_level.begin(), ::tolower);

  spdlog::level::level_enum spd_log_level;
  if (log_level == "off") {
    spd_log_level = spdlog::level::off;
  } else if (log_level == "trace") {
    spd_log_level = spdlog::level::trace;
  } else if (log_level == "debug") {
    spd_log_level = spdlog::level::debug;
  } else if (log_level == "info") {
    spd_log_level = spdlog::level::info;
  } else if (log_level == "warn") {
    spd_log_level = spdlog::level::warn;
  } else if (log_level == "error") {
    spd_log_level = spdlog::level::err;
  } else if (log_level == "critical") {
    spd_log_level = spdlog::level::critical;
  } else {
    std::cerr << "Invalid log level: " << log_level << std::endl;
    throw std::runtime_error("Invalid log level");
  }

  spdlog::set_level(spd_log_level);
}