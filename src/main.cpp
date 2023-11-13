
#include <spdlog/common.h>

#include <algorithm>
#include <iostream>
#include <random>
#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "core/buffer.hpp"
#include "core/engine.hpp"
#include "spdlog/spdlog.h"

namespace core {
VmaAllocator g_allocator;
}

[[nodiscard]] std::ostream &operator<<(std::ostream &os, const glm::vec3 &v) {
  os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
  return os;
}

int main(int argc, char **argv) {
#if defined(NDEBUG)
  spdlog::set_level(spdlog::level::off);
#else
  spdlog::set_level(spdlog::level::debug);
#endif

  constexpr auto min_coord = 0.0f;
  constexpr auto max_coord = 1024.0f;
  constexpr auto range = max_coord - min_coord;
  std::default_random_engine gen(114514);  // NOLINT(cert-msc51-cpp)
  std::uniform_real_distribution dis(min_coord, range);

  constexpr auto n = 1024;
  std::vector<float> in_data(n);
  std::ranges::generate(in_data, [&] { return dis(gen); });

  // Computation start here
  core::ComputeEngine engine{};

  const auto in_buf = engine.yx_buffer(in_data.size() * sizeof(float));
  const auto out_but = engine.yx_buffer(in_data.size() * sizeof(float));

  // in_buf->tmp_write_data(in_data.data(), in_data.size() * sizeof(float));
  in_buf->tmp_debug_data(in_data.size() * sizeof(float));
  out_but->tmp_fill_zero(in_data.size() * sizeof(float));

  std::vector params{in_buf, out_but};

  uint32_t threads_per_block = 256;

  std::vector<float> push_const{0, 0, 0, 0, n};
  const auto algo = engine.yx_algorithm(
      "float_doubler.spv", params, threads_per_block, push_const);

  const auto seq = engine.yx_sequence();

  seq->simple_record_commands(*algo, n);
  seq->launch_kernel_async();

  // ... do something else

  seq->sync();

  // seq->record(*algo);
  // seq->launch_kernel_async();
  // seq->sync();

  const auto in = reinterpret_cast<const float *>(in_buf->get_data());
  const auto out = reinterpret_cast<const float *>(out_but->get_data());
  for (int i = 0; i < n; ++i) {
    std::cout << i << ":\t" << in[i] << "\t-\t" << out[i] << std::endl;
  }

  std::cout << "Done!" << std::endl;
  return EXIT_SUCCESS;
}
