#include <iostream>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "core/base_engine.hpp"
#include "core/compute_shader.hpp"

namespace core {
VmaAllocator allocator;
}

[[nodiscard]] std::ostream &operator<<(std::ostream &os, const glm::vec3 &v) {
  os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
  return os;
}

int main(int argc, char **argv) {
  core::BaseEngine engine{};

  core::ComputeShader shader{engine.get_disp(), "None"};

  return 0;
}
