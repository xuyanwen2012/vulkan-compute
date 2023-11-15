#include <iostream>
#include <vector>

#include "core/engine.hpp"
#include "helpers.hpp"

int main(int argc, char **argv) {
  constexpr auto n = 1024;

  core::ComputeEngine engine{};

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
  for (int i = 0; i < n; ++i) {
    std::cout << i << ":\t" << in[i] << "\t-\t" << out[i] << std::endl;
  }

  std::cout << "Done!" << std::endl;
  return EXIT_SUCCESS;
}
