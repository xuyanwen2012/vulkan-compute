#include <iostream>

#include "core/compute_pipeline.hpp"
#include "core/compute_shader.hpp"
#include "core/engine.hpp"

int main(int argc, char** argv) {
  core::ComputeEngine engine{false};

  core::ComputePipeline pipeline{engine.get_device_shared_ptr()};

  core::ComputeShader shader{
      engine.get_device_shared_ptr(), &pipeline, "vec_add.spv"};

  std::cout << "Done!" << std::endl;
  return EXIT_SUCCESS;
}
