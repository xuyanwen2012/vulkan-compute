#pragma once

#include <fstream>
#include <iostream>
#include <vector>

[[nodiscard]] inline std::vector<uint32_t>
file_reader(const std::string &filename) {
  // Open the file in binary mode
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    std::cerr << "Failed to open file: " << filename << std::endl;
    return {};
  }

  // Get the file size
  size_t file_size = static_cast<size_t>(file.tellg());

  // Read the file into a vector
  std::vector<uint32_t> buffer(file_size / sizeof(uint32_t));
  file.seekg(0);
  file.read(reinterpret_cast<char *>(buffer.data()), file_size);

  file.close();

  if (file.fail()) {
    std::cerr << "Failed to read file: " << filename << std::endl;
    return {};
  }

  return buffer;
}
