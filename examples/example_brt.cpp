#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

#include "morton.hpp"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "core/engine.hpp"
#include "helpers.hpp"
#include "spdlog/spdlog.h"

#define policy_t std::execution::par

[[nodiscard]] std::ostream &operator<<(std::ostream &os, const glm::vec4 &v) {
  os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
  return os;
}

struct InnerNode {
  int32_t delta;
  int32_t left;
  int32_t right;
  int32_t parent;
};

std::ostream &operator<<(std::ostream &os, const InnerNode &node) {
  os << std::left << std::setw(10) << "\tDelta: " << std::right << std::setw(10)
     << node.delta << "\n";
  os << std::left << std::setw(10) << "\tLeft: " << std::right << std::setw(10)
     << node.left << "\n";
  os << std::left << std::setw(10) << "\tRight: " << std::right << std::setw(10)
     << node.right << "\n";
  os << std::left << std::setw(10) << "\tParent: " << std::right
     << std::setw(10) << node.parent << "\n";
  return os;
}

// std::ostream &operator<<(std::ostream &os, const InnerNode &node) {
//   os << "(" << node.delta << ", " << node.left << ", " << node.right << ", "
//      << node.parent << ")";
//   return os;
// }

int main(int argc, char **argv) {
  spdlog::set_level(spdlog::level::debug);

  constexpr auto n = 1024;

  // Computation start here
  core::ComputeEngine engine{};

  // prepare data
  constexpr auto min_coord = 0.0f;
  constexpr auto max_coord = 1024.0f;
  constexpr auto range = max_coord - min_coord;
  std::default_random_engine gen(114514);
  std::uniform_real_distribution dis(min_coord, range);
  std::vector<glm::vec4> in_data(n);
  std::ranges::generate(in_data, [&] {
    return glm::vec4{dis(gen), dis(gen), dis(gen), 0.0f};
  });

  // compute and sort morton

  auto u_morton_keys = std::vector<glm::uint>(n);
  morton::foo(in_data.data(), u_morton_keys.data(), n, min_coord, range);

  std::sort(u_morton_keys.begin(), u_morton_keys.end());

  const auto last_unique_it =
      std::unique(u_morton_keys.begin(), u_morton_keys.end());

  const auto num_unique_keys =
      std::distance(u_morton_keys.begin(), last_unique_it);
  const auto num_brt_nodes = num_unique_keys - 1;

  spdlog::info("num_unique_keys: {}", num_unique_keys);
  // peek the first 10 keys
  for (int i = 0; i < 10; ++i) {
    std::cout << i << ":\t" << u_morton_keys[i] << std::endl;
  }

  //
  const auto morton_key_buf = engine.buffer(n * sizeof(uint32_t));
  const auto inner_nodes_buf = engine.buffer(n * sizeof(InnerNode));

  morton_key_buf->tmp_write_data(u_morton_keys.data(), n * sizeof(uint32_t));
  inner_nodes_buf->tmp_fill_zero(n * sizeof(InnerNode));

  std::vector params{morton_key_buf, inner_nodes_buf};

  constexpr uint32_t threads_per_block = 256;

  auto algo = engine.algorithm("build_radix_tree.spv",
                               params,
                               threads_per_block,
                               true,
                               make_clspv_push_const(n));

  const auto seq = engine.sequence();

  seq->simple_record_commands(*algo, n);
  seq->launch_kernel_async();

  // ... do something else

  seq->sync();

  // Show results
  const auto out =
      reinterpret_cast<const InnerNode *>(inner_nodes_buf->get_data());
  for (int i = 0; i < 10; ++i) {
    std::cout << i << ":\n" << out[i] << std::endl;
  }

  return EXIT_SUCCESS;
}
