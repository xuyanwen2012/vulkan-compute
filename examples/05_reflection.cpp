#include <iostream>

#include "core/compute_shader.hpp"
#include "core/engine.hpp"

int main(int argc, char** argv) {
  core::ComputeEngine engine{false};

  core::ComputeShader shader{engine.get_device_shared_ptr(), "vec_add.spv"};

  std::cout << "Done!" << std::endl;
  return EXIT_SUCCESS;
}
