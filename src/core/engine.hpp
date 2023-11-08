#pragma once

#include <glm/fwd.hpp>
#include <memory>
#include <vulkan/vulkan.hpp>

#include "base_engine.hpp"
#include "buffer.hpp"

// #include "command_buffer.hpp"
// #include "command_pool.hpp"

using InputT = glm::vec4;
using OutputT = glm::uint;

[[nodiscard]] constexpr uint32_t InputSize() { return 1024; }
[[nodiscard]] constexpr uint32_t ComputeShaderProcessUnit() { return 256; }
[[nodiscard]] constexpr uint32_t div_up(const uint32_t x, const uint32_t y) {
  return (x + y - 1u) / y;
}
[[nodiscard]] constexpr uint32_t NumWorkGroup(const uint32_t items) {
  return div_up(items, ComputeShaderProcessUnit());
}

namespace core {

class ComputeEngine : public BaseEngine {
public:
  ComputeEngine() : BaseEngine() {
    vkhDevice = device.device;

    create_command_pool();
  }

  ~ComputeEngine() {
    vkhDevice.freeCommandBuffers(command_pool, command_buffer);
    vkhDevice.destroyCommandPool(command_pool);
  }

  void run(const std::vector<InputT> &input_data) {}

protected:
  void create_command_pool() {
    vk::CommandPoolCreateInfo createInfo;
    createInfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
        .setQueueFamilyIndex(
            device.get_queue_index(vkb::QueueType::compute).value());

    command_pool = vkhDevice.createCommandPool(createInfo);

    vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
    commandBufferAllocateInfo.setCommandBufferCount(1)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandPool(command_pool);

    command_buffer =
        vkhDevice.allocateCommandBuffers(commandBufferAllocateInfo).front();
  }

private:
  vk::Device vkhDevice;

  // Only need one for entire application,
  // otherthings are: instance, physical device, device, queue
  // and global VMA allocator
  vk::CommandPool command_pool;
  // std::vector<vk::CommandPool> m_threadCommandPool;

  vk::CommandBuffer command_buffer; // immediate command buffer
  // These are for each compute shader (pipeline)

  vk::Pipeline pipeline;
  vk::PipelineLayout pipeline_layout;
  vk::DescriptorSetLayout descriptor_set_layout;
  vk::DescriptorPool descriptor_pool;
  vk::DescriptorSet descriptor_set;

  std::vector<Buffer> usm_buffers;

  // std::shared_ptr<DescriptorAllocator> m_descriptorAllocator;
  // std::shared_ptr<DescriptorLayoutCache> m_descriptorLayoutCache;
};

} // namespace core