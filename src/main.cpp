#include <CLI/CLI.hpp>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

#include "morton/tmp.hpp"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "core/engine.hpp"
#include "spdlog/spdlog.h"

[[nodiscard]] std::ostream &operator<<(std::ostream &os, const glm::vec4 &v) {
  os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
  return os;
}

// typename T = float,
template <typename... Args>
[[nodiscard]] std::vector<float> make_clspv_push_const(Args &&...args) {
  return {0.0f, 0.0f, 0.0f, std::forward<Args>(args)...};
}

int main(int argc, char **argv) {
  CLI::App app{"Vulkan Compute Example"};

// Default log level
#if defined(NDEBUG)
  std::string log_level = "off";
#else
  std::string log_level = "debug";
#endif

  app.add_option(
      "-l,--log-level",
      log_level,
      "Set the log level (trace, debug, info, warn, error, critical)");

  int which_example = 0;
  app.add_option("-e,--example",
                 which_example,
                 "Which example to run (0: float doubler, 1: morton code)")
      ->default_val(0);

  CLI11_PARSE(app, argc, argv);

  std::ranges::transform(log_level, log_level.begin(), ::tolower);

  spdlog::level::level_enum spd_log_level;
  if (log_level == "off") {
    spd_log_level = spdlog::level::off;
  } else if (log_level == "trace") {
    spd_log_level = spdlog::level::trace;
  } else if (log_level == "debug") {
    spd_log_level = spdlog::level::debug;
  } else if (log_level == "info") {
    spd_log_level = spdlog::level::info;
  } else if (log_level == "warn") {
    spd_log_level = spdlog::level::warn;
  } else if (log_level == "error") {
    spd_log_level = spdlog::level::err;
  } else if (log_level == "critical") {
    spd_log_level = spdlog::level::critical;
  } else {
    // Handle invalid log level input
    std::cerr << "Invalid log level: " << log_level << std::endl;
    return EXIT_FAILURE;
  }

  spdlog::set_level(spd_log_level);

  constexpr auto n = 1024;

  // Computation start here
  core::ComputeEngine engine{};

  // ---------- Example A ------------
  if (which_example == 0) {
    const auto in_buf = engine.buffer(n * sizeof(float));
    const auto out_but = engine.buffer(n * sizeof(float));

    in_buf->tmp_debug_data(n * sizeof(float));
    out_but->tmp_fill_zero(n * sizeof(float));

    std::vector params{in_buf, out_but};

    uint32_t threads_per_block = 256;

    std::vector<float> push_const{0, 0, 0, 0, n};
    const auto algo = engine.algorithm(
        "float_doubler.spv", params, threads_per_block, push_const);

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

    uint32_t threads_per_block = 256;

    std::vector<float> push_const{0, 0, 0, 0, n, min_coord, range};
    const auto algo =
        engine.algorithm("morton32.spv", params, threads_per_block, push_const);

    const auto seq = engine.sequence();

    seq->simple_record_commands(*algo, n);
    seq->launch_kernel_async();

    // ... do something else

    seq->sync();

    auto cpu_out = std::vector<glm::uint>(n);
    morton::foo(in_data.data(), cpu_out.data(), n, min_coord, range);

    const auto in = reinterpret_cast<const glm::vec4 *>(in_buf->get_data());
    const auto out = reinterpret_cast<const glm::uint *>(out_but->get_data());
    for (int i = 0; i < n; ++i) {
      std::cout << i << ":\t" << in[i] << "\t-\t" << out[i] << "\t("
                << cpu_out[i] << ")" << std::endl;
    }
  }

  std::cout << "Done!" << std::endl;
  return EXIT_SUCCESS;
}
