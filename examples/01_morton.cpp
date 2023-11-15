#include <iostream>
#include <vector>
#include <random>

#include "common.hpp"
#include "core/engine.hpp"
#include "helpers.hpp"
#include "baseline/morton.hpp"


int main(int argc, char **argv) {
  setup_log_level("debug");
  constexpr auto n = 1024;

  core::ComputeEngine engine{};

  constexpr auto min_coord = 0.0f;
  constexpr auto max_coord = 1024.0f;
  constexpr auto range = max_coord - min_coord;
  std::default_random_engine gen(114514);  // NOLINT(cert-msc51-cpp)
  std::uniform_real_distribution dis(min_coord, range);

  std::vector<glm::vec4> in_data(n);
  std::ranges::generate(in_data, [&] {
    return glm::vec4{dis(gen), dis(gen), dis(gen), 0.0f};
  });

  const auto in_buf = engine.buffer(n * sizeof(glm::vec4));
  const auto out_but = engine.buffer(n * sizeof(glm::uint));

  in_buf->tmp_write_data(in_data.data(), n * sizeof(glm::vec4));
  out_but->tmp_fill_zero(n * sizeof(glm::uint));

  std::vector params{in_buf, out_but};

  constexpr uint32_t threads_per_block = 256;

  const auto algo =
      engine.algorithm("morton32.spv",
                       params,
                       threads_per_block,
                       true,
                       make_clspv_push_const(n, min_coord, range));

  const auto seq = engine.sequence();

  seq->simple_record_commands(*algo, n);
  seq->launch_kernel_async();

  // ... do something else

  seq->sync();

  auto cpu_out = std::vector<glm::uint>(n);
  morton::point_to_morton32(in_data.data(), cpu_out.data(), n, min_coord, range);

  const auto in = reinterpret_cast<const glm::vec4 *>(in_buf->get_data());
  const auto out = reinterpret_cast<const glm::uint *>(out_but->get_data());
  for (int i = 0; i < n; ++i) {
    std::cout << i << ":\t" << in[i] << "\t-\t" << out[i] << "\t(" << cpu_out[i]
              << ")" << std::endl;
  }

  std::cout << "Done!" << std::endl;
  return EXIT_SUCCESS;
}
