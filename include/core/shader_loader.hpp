#pragma once

#include <spdlog/spdlog.h>

#include <filesystem>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;

[[nodiscard]] inline std::vector<uint32_t> load_shader_from_file(
    const std::string_view filename) {
  const fs::path shader_path = fs::current_path() / filename;
  spdlog::info("loading shader path: {}", shader_path.string());

  if (!exists(shader_path)) {
    throw std::runtime_error("Shader file not found: " + shader_path.string());
  }

  // Open the shader file
  std::ifstream file(filename.data(), std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + shader_path.string());
  }

  const size_t file_size = file.tellg();

  // Read the file into a vector
  std::vector<uint32_t> buffer(file_size / sizeof(uint32_t));
  file.seekg(0);
  file.read(reinterpret_cast<char *>(buffer.data()), file_size);

  file.close();

  if (file.fail()) {
    throw std::runtime_error("Failed to read file: " + shader_path.string());
  }

  return buffer;
}
