#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>

#include "algorithm.hpp"
#include "base_engine.hpp"
#include "buffer.hpp"
#include "sequence.hpp"

namespace core {
class ComputeEngine : public BaseEngine {
public:
  ComputeEngine() : vkh_device_(device_.device) {}

  ~ComputeEngine() {
    spdlog::debug("ComputeEngine::~ComputeEngine");
    destroy();
  }

  void destroy();

  [[nodiscard]] std::shared_ptr<Buffer> yx_buffer(vk::DeviceSize size) {
    auto buf = std::make_shared<Buffer>(get_device_ptr(), size);
    if (manage_resources_) {
      buffers_.push_back(buf);
    }
    return buf;
  }

  [[nodiscard]] std::shared_ptr<Sequence> yx_sequence() {
    auto seq = std::make_shared<Sequence>(get_device_ptr(), device_, queue_);
    if (manage_resources_) {
      sequence_.push_back(seq);
    }
    return seq;
  }

  template <typename... Args>
  [[nodiscard]] std::shared_ptr<Algorithm> yx_algorithm(Args &&...args) {
    auto algo = std::make_shared<Algorithm>(get_device_ptr(),
                                            std::forward<Args>(args)...);
    if (manage_resources_) {
      algorithms_.push_back(algo);
    }
    return algo;
  }

private:
  vk::Device vkh_device_;

  std::vector<std::weak_ptr<Algorithm>> algorithms_;
  std::vector<std::weak_ptr<Buffer>> buffers_;
  std::vector<std::weak_ptr<Sequence>> sequence_;

  bool manage_resources_ =
      true; // Should the engine manage the above resources?
};
} // namespace core
