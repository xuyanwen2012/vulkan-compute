#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>

#include "algorithm.hpp"
#include "base_engine.hpp"
#include "buffer.hpp"
#include "sequence.hpp"

template <typename T, typename... Args>
concept EngineComponentArgsMatch =
    std::is_constructible_v<T, std::shared_ptr<vk::Device>, Args...>;

namespace core {

/**
 * @brief ComputeEngine is the main class of this library. It provides
 * interfaces for users to create buffers, algorithms, and sequences.
 *
 * It also manages the lifetime of these resources, and frees them when
 * necessary.
 */
class ComputeEngine : public BaseEngine {
 public:
  explicit ComputeEngine(const bool manage_resources = true)
      : vkh_device_(device_.device), manage_resources_(manage_resources) {}

  ~ComputeEngine() {
    spdlog::debug("ComputeEngine::~ComputeEngine");
    destroy();
  }

  void destroy();

  /**
   * @brief It sucks, but this function takes the N*sizeof(T)
   *
   * @param size
   * @return std::shared_ptr<Buffer>
   */
  [[nodiscard]] std::shared_ptr<Buffer> buffer(vk::DeviceSize size) {
    auto buf = std::make_shared<Buffer>(get_device_shared_ptr(), size);
    if (manage_resources_) {
      buffers_.push_back(buf);
    }
    return buf;
  }

  [[nodiscard]] std::shared_ptr<Sequence> sequence() {
    auto seq =
        std::make_shared<Sequence>(get_device_shared_ptr(), device_, queue_);
    if (manage_resources_) {
      sequence_.push_back(seq);
    }
    return seq;
  }

  /**
   * @brief Creates a new instance of the Algorithm class with the given
   * arguments.
   *
   * @tparam Args The types of the arguments to pass to the Algorithm
   * constructor.
   * @param args The arguments to pass to the Algorithm constructor.
   * @return std::shared_ptr<Algorithm> A shared pointer to the newly created
   * Algorithm instance.
   * @throws std::invalid_argument if the given arguments do not match the
   * required EngineComponentArgs for Algorithm.
   */
  template <typename... Args>
  [[nodiscard]] auto algorithm(Args &&...args) -> std::shared_ptr<Algorithm>
    requires EngineComponentArgsMatch<Algorithm, Args...>
  {
    auto algo = std::make_shared<Algorithm>(get_device_shared_ptr(),
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

  /**
   * @brief Should the engine manage the above resources?
   */
  bool manage_resources_ = true;
};
}  // namespace core
