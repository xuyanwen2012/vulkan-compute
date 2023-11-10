#pragma once

#include "vulkan_resource.hpp"

namespace core {

class Sequence final : public VulkanResource<vk::CommandBuffer> {

  Sequence(std::shared_ptr<vk::Device> device_ptr,
           const vk::CommandBuffer command_buffer)
      : VulkanResource(std::move(device_ptr)) {

    create_command_pool();
    create_command_buffer();
  }

  ~Sequence() override { destroy(); }

  void begin();

  void end();
  void submit();

protected:
  void destroy() override {
    if (handle_ != VK_NULL_HANDLE) {
      device_ptr_->freeCommandBuffers(command_pool_, handle_);
    }
  }

  void create_command_pool(){
    //  const auto create_info =
    //     vk::CommandPoolCreateInfo()
    //         .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
    //         .setQueueFamilyIndex(
    //             device_.get_queue_index(vkb::QueueType::compute).value());
  }

  void create_command_buffer();
  
  void create_timestamp_query_pool(uint32_t totalTimestamps);

private:
  vk::CommandPool command_pool_;
  vk::Fence fence_;
  vk::QueryPool timestamp_query_pool_;
};

} // namespace core
