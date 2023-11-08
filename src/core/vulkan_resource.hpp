#pragma once

#include <vulkan/vulkan.hpp>

namespace core {

template <typename HandleT> class VulkanResource {
public:
  VulkanResource() = default;
  VulkanResource(const VulkanResource &) = delete;
  VulkanResource(VulkanResource &&) = delete;
  virtual ~VulkanResource() = default;

  VulkanResource &operator=(const VulkanResource &) = delete;
  VulkanResource &operator=(VulkanResource &&) = delete;

  virtual void destroy() = 0;

private:
  VkDevice *device;
  HandleT handle;
};
}; // namespace core