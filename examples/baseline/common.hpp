#pragma once

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>
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

inline void save_pod_data_to_file(
    const std::byte *data,
    const size_t size,
    const std::string_view filename = "data.bin") {
  std::ofstream out_file(filename.data(), std::ios::binary);

  if (!out_file.is_open()) {
    throw std::runtime_error("Failed to open file for writing.");
  }

  spdlog::info("Saving {} bytes to {}", size, filename);

  out_file.write(reinterpret_cast<const char *>(data), size);
  out_file.close();
}

template <typename T>
inline void save_pod_data_to_file(const std::vector<T> &data,
                                  const std::string &filename = "data.bin") {
  save_pod_data_to_file(reinterpret_cast<const std::byte *>(data.data()),
                        data.size() * sizeof(T),
                        filename);
}

template <typename T>
[[nodiscard]] inline std::vector<T> load_pod_data_from_file(
    const std::string_view filename) {
  std::vector<T> loaded_data;

  std::ifstream in_file(filename.data(), std::ios::binary);
  if (!in_file.is_open()) {
    throw std::runtime_error("Failed to open file for reading.");
  }

  in_file.seekg(0, std::ios::end);
  size_t file_size = in_file.tellg();
  in_file.seekg(0, std::ios::beg);

  loaded_data.resize(file_size / sizeof(T));

  in_file.read(reinterpret_cast<char *>(loaded_data.data()), file_size);
  in_file.close();

  return loaded_data;
}
