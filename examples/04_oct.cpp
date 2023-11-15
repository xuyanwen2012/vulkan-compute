#include <cstdint>
#include <filesystem>
#include <iostream>
#include <vector>

#include "baseline/brt.hpp"
#include "common.hpp"
#include "core/engine.hpp"

int main(int argc, char **argv) {
  setup_log_level("debug");

  // Computation start here
  core::ComputeEngine engine{};

  constexpr auto n = 1024;

  // print current working directory

  spdlog::info("Current working directory: {}",
               std::filesystem::current_path().string());

  auto mortons = load_pod_data_from_file<uint32_t>("sorted_mortons_1024.bin");
  auto brt_nodes =
      load_pod_data_from_file<brt::InnerNode>("brt_nodes_1024.bin");

  for (int i = 0; i < 10; ++i) {
    std::cout << i << ":\t" << brt_nodes[i] << std::endl;
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
