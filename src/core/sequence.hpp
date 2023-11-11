#pragma once

#include "VkBootstrap.h"
#include "vulkan_resource.hpp"
#include "yx_algorithm.hpp"
#include <memory>
#include <vulkan/vulkan_handles.hpp>

namespace core {

[[nodiscard]] constexpr uint32_t num_blocks(const uint32_t items,
                                            const uint32_t threads_per_block) {
  return (items + threads_per_block - 1u) / threads_per_block;
}

class Sequence final : public VulkanResource<vk::CommandBuffer> {
public:
  Sequence(std::shared_ptr<vk::Device> device_ptr,
           const vkb::Device &vkb_device,
           vk::Queue &queue) // tmp
      : VulkanResource(std::move(device_ptr)), vkb_device_(vkb_device),
        vkh_queue_(&queue) {
    create_sync_objects();
    create_command_pool();
    create_command_buffer();
  }

  ~Sequence() override { destroy(); }

  void record(YxAlgorithm &algo) {
    begin();
    algo.record_bind_core(handle_);
    algo.record_bind_push(handle_);
    algo.record_dispatch(handle_);
    end();
  }

  void begin() {
    spdlog::debug("Sequence::begin!");
    const auto info = vk::CommandBufferBeginInfo().setFlags(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    handle_.begin(info);
  }

  void end() {
    spdlog::debug("Sequence::end!");
    handle_.end();
  }

  // void push_constants(const vk::Pipeline &pipeline, const std::byte *data,
  //                     const size_t size) {}

  void launch_kernel_async() {
    spdlog::debug("Sequence::launch_kernel_async");
    const auto submit_info = vk::SubmitInfo().setCommandBuffers(handle_);
    assert(vkh_queue_ != nullptr);
    vkh_queue_->submit(submit_info, fence_);
  }

  void sync() {
    spdlog::debug("Sequence::sync");
    const auto wait_result =
        device_ptr_->waitForFences(fence_, false, UINT64_MAX);
    assert(wait_result == vk::Result::eSuccess);
    device_ptr_->resetFences(fence_);
  }

  void destroy() override {
    device_ptr_->freeCommandBuffers(command_pool_, handle_);
    device_ptr_->destroyCommandPool(command_pool_);
    device_ptr_->destroyFence(fence_);
  }

protected:
  void create_sync_objects() {
    fence_ = device_ptr_->createFence(vk::FenceCreateInfo());
  }

  void create_command_pool() {
    spdlog::debug("Sequence::create_command_pool");

    const auto create_info =
        vk::CommandPoolCreateInfo()
            .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
            .setQueueFamilyIndex(
                vkb_device_.get_queue_index(vkb::QueueType::compute).value());

    command_pool_ = device_ptr_->createCommandPool(create_info);
  }

  void create_command_buffer() {
    spdlog::debug("Sequence::create_command_buffer");

    const auto alloc_info = vk::CommandBufferAllocateInfo()
                                .setCommandBufferCount(1)
                                .setLevel(vk::CommandBufferLevel::ePrimary)
                                .setCommandPool(command_pool_);

    handle_ = device_ptr_->allocateCommandBuffers(alloc_info).front();
  }

  void create_timestamp_query_pool(uint32_t totalTimestamps);

private:
  const vkb::Device &vkb_device_;
  vk::Queue *vkh_queue_;

  // std::weak_ptr<vk::Queue> queue_;
  // vk::Queue *vkh_queue_;
  // vkb::Queue queue_;;

  vk::CommandPool command_pool_;
  vk::Fence fence_;
  vk::QueryPool timestamp_query_pool_;
};

} // namespace core
