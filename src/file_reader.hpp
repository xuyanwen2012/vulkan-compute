#pragma once

#include <cstring>
#include <fstream>
#include <vector>



namespace io {
[[nodiscard]] inline std::vector<uint32_t>
ReadSpirvBinaryFile(std::string_view filename) {
  std::ifstream file(filename.data(), std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file with file path " +
                             std::string(filename));
  }

  std::vector<char> spirv((std::istreambuf_iterator<char>(file)),
                          (std::istreambuf_iterator<char>()));

  file.close();

  // Copy data from the char-vector to a new uint32_t-vector
  std::vector<uint32_t> spv(spirv.size() / sizeof(uint32_t));
  memcpy(spv.data(), spirv.data(), spirv.size());

  return spv;
}
} // namespace io

[[nodiscard]] inline std::vector<char> readFile(const std::string &filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("failed to open file!");
  }

  size_t file_size = (size_t)file.tellg();
  std::vector<char> buffer(file_size);

  file.seekg(0);
  file.read(buffer.data(), static_cast<std::streamsize>(file_size));

  file.close();

  return buffer;
}
