#include <CLI/CLI.hpp>
#include <algorithm>
#include <iostream>
#include <random>
#include <vector>

#include "baseline/morton.hpp"
#include "common.hpp"
#include "core/engine.hpp"
#include "helpers.hpp"

int main(int argc, char **argv) {
  CLI::App app{"Vulkan Compute Example"};

  std::string log_level = "debug";
  app.add_option(
         "-l,--log-level",
         log_level,
         "Set the log level (trace, debug, info, warn, error, critical)")
      ->default_val("debug");

  int which_example;
  app.add_option("-e,--example",
                 which_example,
                 "Which example to run (0: float doubler, 1: morton code)")
      ->default_val(0);

  CLI11_PARSE(app, argc, argv);

  setup_log_level(log_level);

  constexpr auto n = 1024;

  core::ComputeEngine engine{};

  // ---------- Example A ------------
  if (which_example == 0) {
    const auto in_buf = engine.buffer(n * sizeof(float));
    const auto out_but = engine.buffer(n * sizeof(float));

    in_buf->tmp_debug_data<float>(n * sizeof(float));
    out_but->tmp_fill_zero(n * sizeof(float));

    std::vector params{in_buf, out_but};

    uint32_t threads_per_block = 256;

    auto algo = engine.algorithm("float_doubler.spv",
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
    const auto in = reinterpret_cast<const float *>(in_buf->get_data());
    const auto out = reinterpret_cast<const float *>(out_but->get_data());
    for (int i = 0; i < 10; ++i) {
      std::cout << i << ":\t" << in[i] << "\t-\t" << out[i] << std::endl;
    }
    std::cout << "..." << std::endl;
    for (int i = n - 10; i < n; ++i) {
      std::cout << i << ":\t" << in[i] << "\t-\t" << out[i] << std::endl;
    }
  }

  // ---------- Example B ------------
  if (which_example == 1) {
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
    morton::point_to_morton32(
        in_data.data(), cpu_out.data(), n, min_coord, range);

    const auto in = reinterpret_cast<const glm::vec4 *>(in_buf->get_data());
    const auto out = reinterpret_cast<const glm::uint *>(out_but->get_data());
    for (int i = 0; i < n; ++i) {
      std::cout << i << ":\t" << in[i] << "\t-\t" << out[i] << "\t("
                << cpu_out[i] << ")" << std::endl;
    }
  }

  if (which_example == 2) {
    using InputT = uint32_t;
    using OutputT = uint32_t;

    const auto in_buf = engine.buffer(n * sizeof(InputT));
    const auto out_but = engine.buffer(n * sizeof(OutputT));

    std::vector<InputT> in_data(n);
    std::iota(in_data.begin(), in_data.end(), 0);
    std::reverse(in_data.begin(), in_data.end());

    in_buf->tmp_write_data(in_data.data(), n * sizeof(InputT));
    out_but->tmp_fill_zero(n * sizeof(OutputT));

    std::vector params{in_buf, out_but};

    uint32_t threads_per_block = 256;

    const auto algo = engine.algorithm("radix_sort.spv",
                                       params,
                                       threads_per_block,
                                       true,
                                       make_clspv_push_const(n));

    const auto seq = engine.sequence();

    // seq->simple_record_commands(*algo, n);
    seq->cmd_begin();
    algo->record_bind_core(seq->get_handle());
    algo->record_bind_push(seq->get_handle());
    algo->record_dispatch_tmp(seq->get_handle(), 256);
    seq->cmd_end();

    seq->launch_kernel_async();

    // ... do something else

    seq->sync();

    // Show results
    const auto in = reinterpret_cast<const InputT *>(in_buf->get_data());
    const auto out = reinterpret_cast<const OutputT *>(out_but->get_data());
    for (int i = 0; i < 256; ++i) {
      std::cout << i << ":\t" << in[i] << "\t-\t" << out[i] << std::endl;
    }
  }

  std::cout << "Done!" << std::endl;
  return EXIT_SUCCESS;
}
