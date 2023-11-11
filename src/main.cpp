
#include <algorithm>
#include <iostream>
#include <memory>
#include <random>
#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "core/buffer.hpp"
#include "core/engine.hpp"
// #include "core/sequence.hpp"

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
  spdlog::set_level(spdlog::level::trace);
#endif

  constexpr auto min_coord = 0.0f;
  constexpr auto max_coord = 1024.0f;
  constexpr auto range = max_coord - min_coord;
  std::default_random_engine gen(114514); // NOLINT(cert-msc51-cpp)
  std::uniform_real_distribution dis(min_coord, range);

  constexpr auto n = 1024;
  std::vector<float> in_data(n);
  std::ranges::generate(in_data, [&] { return dis(gen); });

  // Computation start here
  core::ComputeEngine engine{};

  auto in_buf = engine.yx_buffer(n);
  auto out_but = engine.yx_buffer(n);

  in_buf->tmp_write_data(reinterpret_cast<void *>(in_data.data()),
                         in_data.size() * sizeof(float));

  out_but->tmp_fill_zero(in_data.size() * sizeof(float));

  std::vector<std::shared_ptr<core::Buffer>> params{in_buf, out_but};

  // I still want to know what workgroup means
  // std::array<uint32_t, 3> workgroup{core::num_blocks(n, 256), 1, 1};
  core::WorkGroup workgroup_size{32, 1, 1};

  std::vector<float> push_const{0, 0, 0, 0, n};
  auto algo =
      engine.yx_algorithm("float_doubler", params, workgroup_size, push_const);

  auto seq = engine.yx_sequence();
  seq->record(*algo);
  seq->launch_kernel_async();
  seq->sync();

  auto o = reinterpret_cast<const float *>(out_but->get_data());
  for (int i = 0; i < n; ++i) {
    std::cout << i << ":\t" << in_data[i] << "\t-\t" << o[i] << std::endl;
  }

  std::cout << "Done!" << std::endl;
  return EXIT_SUCCESS;
}
