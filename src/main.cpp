#include <algorithm>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "core/engine.hpp"

#include "core/yx_algorithm.hpp"

namespace core {
VmaAllocator g_allocator;
}

[[nodiscard]] std::ostream &operator<<(std::ostream &os, const glm::vec3 &v) {
  os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
  return os;
}

int main(int argc, char **argv) {
  constexpr auto min_coord = 0.0f;
  constexpr auto max_coord = 1024.0f;
  constexpr auto range = max_coord - min_coord;
  std::default_random_engine gen(114514); // NOLINT(cert-msc51-cpp)
  std::uniform_real_distribution dis(min_coord, range);

  std::vector<InputT> h_data(InputSize());
  std::ranges::generate(
      h_data, [&] { return glm::vec4(dis(gen), dis(gen), dis(gen), 0.0f); });

  core::ComputeEngine engine{};

  auto algo = engine.yx_algorithm();
  algo->set_push_constants(h_data);
  algo->set_workgroup({32, 1, 1});

  // auto algo = engine.algorithm({32, 1, 1}, {1, 1, 1}, {1, 1, 1});
  // algo->set_push_constants(tmp);
  // algo->set_push_constants(tmp.data(), tmp.size(), sizeof(float));
  // algo->set_workgroup({32, 1, 1});

  // auto algo =

  // engine.run(h_data);

  const auto output_data =
      reinterpret_cast<const OutputT *>(engine.usm_buffers_[1]->get_data());

  std::cout << "Output:" << std::endl;
  for (size_t i = 0; i < 10; ++i) {
    // clang-format off
		std::cout << "[" << i << "] "
			<< std::fixed << std::setprecision(3) << "("
			<< std::setw(8) << h_data[i].x << ", "
			<< std::setw(8) << h_data[i].y << ", "
			<< std::setw(8) << h_data[i].z << ")"
			<< "\t" << std::setw(9) << output_data[i] << std::endl;
    // clang-format on
  }

  std::vector<float> test(100);
  std::ranges::generate(test, [&] { return dis(gen); });

  // core::Tensor tensor(engine.get_device_ptr(), test.data(), test.size(),
  //                     sizeof(decltype(test)::value_type));

  // std::cout << "tensor: " << tensor << std::endl;

  std::cout << "Done!" << std::endl;
  return EXIT_SUCCESS;
}
