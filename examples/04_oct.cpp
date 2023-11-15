#include <cstdint>
#include <filesystem>
#include <iostream>
#include <vector>

#include "baseline/brt.hpp"
#include "baseline/oct.hpp"
#include "common.hpp"
#include "core/engine.hpp"
#include "helpers.hpp"

int main(int argc, char **argv) {
  setup_log_level("debug");
  spdlog::info("Current working directory: {}",
               std::filesystem::current_path().string());

  core::ComputeEngine engine{};

  auto u_mortons = load_pod_data_from_file<uint32_t>("sorted_mortons_1024.bin");
  auto u_brt_nodes =
      load_pod_data_from_file<brt::InnerNode>("brt_nodes_1024.bin");

  constexpr auto n = 1024;
  constexpr auto num_brt_nodes = n - 1;
  assert(u_mortons.size() == n);
  assert(u_brt_nodes.size() == num_brt_nodes);

  std::vector<int32_t> cpu_edge_counts(n);
  std::vector<int32_t> cpu_oc_offset(n);

  cpu_edge_counts[0] = 1u;
  for (int i = 0; i < num_brt_nodes; ++i) {
    oct::CalculateEdgeCountHelper(
        i, cpu_edge_counts.data(), u_brt_nodes.data());
  }

  std::partial_sum(cpu_edge_counts.begin(),
                   cpu_edge_counts.end(),
                   cpu_oc_offset.begin() + 1);
  cpu_oc_offset[0] = 0;

  // ---------------------------------------------------------------------------
  //                          Edge Count
  // ---------------------------------------------------------------------------

  const auto brt_nodes_buf = engine.buffer(n * sizeof(brt::InnerNode));
  const auto edge_count_buf = engine.buffer(n * sizeof(int32_t));

  auto brt_node_ptr = brt_nodes_buf->get_data_mut<brt::InnerNode>();
  auto edge_count_ptr = edge_count_buf->get_data_mut<int32_t>();

  std::ranges::copy(u_brt_nodes, brt_node_ptr);
  edge_count_buf->tmp_fill_zero(n * sizeof(int32_t));

  std::vector params{brt_nodes_buf, edge_count_buf};
  auto algo = engine.algorithm("edge_count.spv",
                               params,
                               256,  // num_threads
                               true,
                               make_clspv_push_const(n));

  const auto seq = engine.sequence();

  seq->simple_record_commands(*algo, n);
  seq->launch_kernel_async();

  // ... do something else

  seq->sync();

  // Verify edge count results
  for (int i = 0; i < num_brt_nodes; ++i) {
    if (cpu_edge_counts[i] != edge_count_ptr[i]) {
      spdlog::error("Mismatch at {} : {} vs {}",
                    i,
                    cpu_edge_counts[i],
                    edge_count_ptr[i]);
    }
  }

  // ---------------------------------------------------------------------------
  //                          Build Octree
  // ---------------------------------------------------------------------------

  spdlog::info("Done");
  return EXIT_SUCCESS;
}
