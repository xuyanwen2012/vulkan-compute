#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>

namespace core {

template <typename HandleT> class VulkanResource {
public:
  VulkanResource() = delete;
  explicit VulkanResource(const std::shared_ptr<vk::Device>& device_ptr) : device_ptr_(device_ptr) {}

  VulkanResource(const VulkanResource &) = delete;
  VulkanResource(VulkanResource &&) = delete;
  virtual ~VulkanResource() = default;

  VulkanResource &operator=(const VulkanResource &) = delete;
  VulkanResource &operator=(VulkanResource &&) = delete;

  HandleT &get_handle() { return handle_; }
  [[nodiscard]] const HandleT  &get_handle() const { return handle_; }

protected:
  std::shared_ptr<vk::Device> device_ptr_;
  HandleT handle_;
};

}; // namespace core
