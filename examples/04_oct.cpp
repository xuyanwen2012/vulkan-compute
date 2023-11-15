#include <cstdint>
#include <filesystem>
#include <iostream>
#include <vector>

#include "baseline/brt.hpp"
#include "baseline/oct.hpp"
#include "common.hpp"
#include "core/engine.hpp"

int main(int argc, char **argv) {
  setup_log_level("debug");
  spdlog::info("Current working directory: {}",
               std::filesystem::current_path().string());

  // Computation start here
  core::ComputeEngine engine{};

  auto u_mortons = load_pod_data_from_file<uint32_t>("sorted_mortons_1024.bin");
  auto u_brt_nodes =
      load_pod_data_from_file<brt::InnerNode>("brt_nodes_1024.bin");

  constexpr auto n = 1024;
  constexpr auto num_brt_nodes = n - 1;

  assert(u_mortons.size() == n);
  assert(u_brt_nodes.size() == num_brt_nodes);

  std::vector<int32_t> u_edge_counts(n);
  std::vector<int32_t> u_oc_offset(n);

  //  std::vector<int> indices(input_size);

  u_edge_counts[0] = 1u;
  for (int i = 0; i < num_brt_nodes; ++i) {
    oct::CalculateEdgeCountHelper(i, u_edge_counts.data(), u_brt_nodes.data());
  }

  std::partial_sum(
      u_edge_counts.begin(), u_edge_counts.end(), u_oc_offset.begin() + 1);
  u_oc_offset[0] = 0;

  for (int i = 0; i < num_brt_nodes; ++i) {
    std::cout << i << ":\t" << u_oc_offset[i] << std::endl;
  }

  

  // const auto in_buf = engine.buffer(n * sizeof(float));
  // const auto out_buf = engine.buffer(n * sizeof(float));

  // // Fill buffer with some data
  // auto in_ptr = in_buf->get_data_mut<float>();
  // auto out_ptr = out_buf->get_data_mut<float>();

  // for (int i = 0; i < n; ++i) {
  //   in_ptr[i] = i;
  //   out_ptr[i] = 0;
  // }

  // std::vector params{in_buf, out_buf};
  // auto algo = engine.algorithm("reduction-0.spv",
  //                              params,
  //                              256,  // num_threads
  //                              true,
  //                              std::vector<float>{0, 0, 0, 0, 0, 0, 0, 0,
  //                              n});
  // const auto seq = engine.sequence();

  // seq->simple_record_commands(*algo, n);
  // seq->launch_kernel_async();

  // // ... do something else

  // seq->sync();

  // // Show results
  // for (int i = 0; i < 10; ++i) {
  //   std::cout << i << ":\t" << out_ptr[i] << std::endl;
  // }

  spdlog::info("Done");
  return EXIT_SUCCESS;
}
