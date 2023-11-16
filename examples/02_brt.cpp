#include <algorithm>
#include <iostream>
#include <random>
#include <vector>

#include "baseline/brt.hpp"
#include "baseline/common.hpp"
#include "baseline/morton.hpp"
#include "core/engine.hpp"
#include "helpers.hpp"

int main(int argc, char **argv) {
  setup_log_level("debug");

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
  morton::point_to_morton32(
      in_data.data(), u_morton_keys.data(), n, min_coord, range);

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
  const auto inner_nodes_buf = engine.buffer(n * sizeof(brt::InnerNode));

  {
    auto ptr = morton_key_buf->get_data_mut<uint32_t>();
    std::ranges::copy(u_morton_keys, ptr);
  }

  {
    auto ptr = inner_nodes_buf->get_data_mut<brt::InnerNode>();
    std::fill_n(ptr, n, brt::InnerNode{});
  }

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
      reinterpret_cast<const brt::InnerNode *>(inner_nodes_buf->get_data());
  for (int i = 0; i < 10; ++i) {
    std::cout << i << ":\n" << out[i] << std::endl;
  }

  std::vector<brt::InnerNode> cpu_out(num_brt_nodes);
  for (auto i = 0; i < num_brt_nodes; ++i) {
    process_inner_node_helper(n, u_morton_keys.data(), i, cpu_out.data());
  }
  for (int i = 0; i < 10; ++i) {
    std::cout << i << " (cpu):\n" << cpu_out[i] << std::endl;
  }

  for (size_t i = 0; i < num_brt_nodes; ++i) {
    if (cpu_out[i].delta_node != out[i].delta_node ||
        cpu_out[i].left != out[i].left || cpu_out[i].right != out[i].right ||
        cpu_out[i].parent != out[i].parent) {
      spdlog::error("Found a difference at index {}", i);
    }
  }

  // Debug
  for (auto i : {528, 529}) {
    std::cout << "CPU Result: \n";
    std::cout << cpu_out[i] << std::endl;
    std::cout << "GPU Result: \n";
    std::cout << out[i] << std::endl;
  }

  // save_pod_data_to_file(u_morton_keys, "sorted_mortons_1024.bin");
  // save_pod_data_to_file(cpu_out, "brt_nodes_1024.bin");

  spdlog::info("Done");
  return EXIT_SUCCESS;
}
