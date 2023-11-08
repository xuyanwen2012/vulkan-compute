#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>

#include "command_buffer.hpp"

namespace core {

class HPPCommandPool {

public:
  HPPCommandPool(std::shared_ptr<vk::Device> device) : device_ptr(device) {
    vk::CommandPoolCreateFlags flags;

    flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    vk::CommandPoolCreateInfo command_pool_create_info(flags);

    if (auto result = device_ptr->createCommandPool(&command_pool_create_info,
                                                    nullptr, &handle);
        result != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to create command pool");
    }
  }
  HPPCommandPool(const HPPCommandPool &) = delete;
  HPPCommandPool(HPPCommandPool &&other);
  ~HPPCommandPool() {
    primary_command_buffers.clear();
    if (handle) {
      device_ptr->destroyCommandPool(handle);
    }
  };

  HPPCommandPool &operator=(const HPPCommandPool &) = delete;
  HPPCommandPool &operator=(HPPCommandPool &&) = delete;

  HPPCommandBuffer &request_command_buffer(
      vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) {

    if (active_primary_command_buffer_count < primary_command_buffers.size()) {
      return *primary_command_buffers[active_primary_command_buffer_count++];
    }

    primary_command_buffers.emplace_back(
        std::make_unique<HPPCommandBuffer>(*this, level));

    active_primary_command_buffer_count++;

    return *primary_command_buffers.back();
  }
  void reset_pool() {
    // case HPPCommandBuffer::ResetMode::ResetPool:
    device_ptr->resetCommandPool(handle);
    reset_command_buffers();
  }

  void reset_command_buffers() {
    for (auto &cmd_buf : primary_command_buffers) {
      cmd_buf->reset();
    }
    active_primary_command_buffer_count = 0;
  }

private:
  std::vector<std::unique_ptr<HPPCommandBuffer>> primary_command_buffers;
  uint32_t active_primary_command_buffer_count = 0;

  // Handles
  vk::CommandPool handle;
  std::shared_ptr<vk::Device> device_ptr;
};

} // namespace core