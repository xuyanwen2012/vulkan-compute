#pragma once

#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>

#include "VkBootstrap.h"
#include "vk_mem_alloc.h"

namespace core {

// Need a global allocator for VMA
extern VmaAllocator g_allocator;

/**
 * @brief BaseEngine setup the Vulkan instance, physical device, logical device
 * etc. For compute shader only.
 *
 */
class BaseEngine {
public:
  BaseEngine();

  ~BaseEngine() {
    spdlog::debug("BaseEngine::~BaseEngine");
    destroy();
  }

  void destroy() const;

  [[nodiscard]] vkb::Device &get_device() { return device_; }

  [[nodiscard]] auto get_device_ptr() {
    return std::make_shared<vk::Device>(device_.device);
  }

  [[nodiscard]] vk::Queue &get_queue() { return queue_; }
  [[nodiscard]] const vk::Queue &get_queue() const { return queue_; }

private:
  void device_initialization();
  void get_queues();
  void vma_initialization() const;

protected:
  vkb::Instance instance_;
  vkb::Device device_;
  vk::Queue queue_;
};
} // namespace core
