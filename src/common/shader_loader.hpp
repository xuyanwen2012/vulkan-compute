#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <spdlog/spdlog.h>
#include <vector>

namespace fs = std::filesystem;

[[nodiscard]] inline std::vector<uint32_t>
load_shader_from_file(const std::string &filename) {

  fs::path shaderPath = fs::current_path() / filename;
  spdlog::info("loading shader path: {}", shaderPath.string());

  if (!fs::exists(shaderPath)) {
    throw std::runtime_error("Shader file not found: " + shaderPath.string());
  }

  // Open the shader file
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + shaderPath.string());
  }

  size_t file_size = static_cast<size_t>(file.tellg());

  // Read the file into a vector
  std::vector<uint32_t> buffer(file_size / sizeof(uint32_t));
  file.seekg(0);
  file.read(reinterpret_cast<char *>(buffer.data()), file_size);

  file.close();

  if (file.fail()) {
    throw std::runtime_error("Failed to read file: " + shaderPath.string());
  }

  return buffer;
}
