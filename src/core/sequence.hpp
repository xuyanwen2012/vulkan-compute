#pragma once

#include "VkBootstrap.h"
#include "algorithm.hpp"
#include "vulkan_resource.hpp"

namespace core {

/**
 * @brief Sequence class is an abstraction of a Vulkan command buffer. It
 * handles command pool/buffer, synchronization, etc.
 * It provides begin/end/record methods for recording commands.
 *
 * You can attached an Algorithm to a Sequence, and call record() to record the
 * commands. It bind pipeline, push constants, and dispatch.
 */
class Sequence final : public VulkanResource<vk::CommandBuffer> {
 public:
  explicit Sequence(std::shared_ptr<vk::Device> device_ptr,
                    const vkb::Device &vkb_device,
                    vk::Queue &queue)  // tmp
      : VulkanResource(std::move(device_ptr)),
        vkb_device_(vkb_device),
        vkh_queue_(&queue) {
    create_sync_objects();
    create_command_pool();
    create_command_buffer();
  }

  ~Sequence() override { destroy(); }

  [[deprecated("Too complicated")]] void record(const Algorithm &algo) const;

  void launch_kernel_async();
  void sync() const;

  void simple_record_commands(const Algorithm &algo, const uint32_t n) const {
    cmd_begin();
    algo.record_bind_core(handle_);
    algo.record_bind_push(handle_);
    algo.record_dispatch_tmp(handle_, n);
    cmd_end();
  }

  void destroy() override;

 protected:
  void create_sync_objects();
  void create_command_pool();
  void create_command_buffer();

  void cmd_begin() const;
  void cmd_end() const;

 private:
  const vkb::Device &vkb_device_;
  vk::Queue *vkh_queue_;

  vk::CommandPool command_pool_;
  vk::Fence fence_;
};

}  // namespace core
