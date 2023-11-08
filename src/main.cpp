#include <iostream>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "core/base_engine.hpp"
#include "core/compute_shader.hpp"

#include "core/buffer.hpp"

namespace core {
VmaAllocator g_allocator;
}

[[nodiscard]] std::ostream &operator<<(std::ostream &os, const glm::vec3 &v) {
  os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
  return os;
}

int main(int argc, char **argv) {
  core::BaseEngine engine{};

  auto buf = core::Buffer(1024 * sizeof(glm::vec4));

  core::ComputeShader shader{engine.get_disp(), "None"};

  return 0;
}
