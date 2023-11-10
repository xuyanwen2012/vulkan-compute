#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vulkan/vulkan.hpp>

#include "base_engine.hpp"
#include "buffer.hpp"
#include "yx_algorithm.hpp"

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

struct MyPushConst {
  uint32_t n;
  float min_coord;
  float range;
};

namespace core {
class ComputeEngine : public BaseEngine {
public:
  ComputeEngine() {
    vkh_device_ = device_.device;
    create_sync_object();
    create_command_pool();
  }

  ~ComputeEngine() {
    spdlog::debug("ComputeEngine::~ComputeEngine");
    destroy();
  }

  void destroy() {
    if (manage_resources_ && !yx_algorithms_.empty()) {
      spdlog::debug("ComputeEngine::destroy() explicitly freeing algorithms");
      for (auto &weak_algorithm : yx_algorithms_) {
        if (const auto algorithm = weak_algorithm.lock()) {
          algorithm->destroy();
        }
      }
      yx_algorithms_.clear();
    }

    if (manage_resources_ && !yx_buffers_.empty()) {
      spdlog::debug("ComputeEngine::destroy() explicitly freeing buffers");
      for (auto &weak_buffer : yx_buffers_) {
        if (const auto buffer = weak_buffer.lock()) {
          buffer->destroy();
        }
      }
      yx_buffers_.clear();
    }

    vkh_device_.destroyFence(immediate_fence_);
    vkh_device_.freeCommandBuffers(command_pool_, immediate_command_buffer_);
    vkh_device_.destroyCommandPool(command_pool_);
  }

  [[nodiscard]] std::shared_ptr<Buffer> yx_buffer(vk::DeviceSize size) {
    auto buf = std::make_shared<Buffer>(get_device_ptr(), size);
    if (manage_resources_) {
      yx_buffers_.push_back(buf);
    }
    return buf;
  }

  template <typename... Args>
  [[nodiscard]] std::shared_ptr<YxAlgorithm> yx_algorithm(Args &&...args) {
    auto algo = std::make_shared<YxAlgorithm>(get_device_ptr(),
                                              std::forward<Args>(args)...);
    if (manage_resources_) {
      yx_algorithms_.push_back(algo);
    }
    return algo;
  }

  void submit_and_wait(const vk::CommandBuffer &command_buffer) const {
    const auto submit_info = vk::SubmitInfo().setCommandBuffers(command_buffer);

    queue_.submit(submit_info, immediate_fence_);

    // wait
    const auto wait_result =
        vkh_device_.waitForFences(immediate_fence_, false, UINT64_MAX);
    assert(wait_result == vk::Result::eSuccess);
    vkh_device_.resetFences(immediate_fence_);
  }

  void run(const std::vector<InputT> &input_data) {
    // execute_sync
    const auto cmd_buf_alloc_info =
        vk::CommandBufferAllocateInfo()
            .setCommandPool(command_pool_)
            .setLevel(vk::CommandBufferLevel::ePrimary)
            .setCommandBufferCount(1);

    immediate_command_buffer_ =
        vkh_device_.allocateCommandBuffers(cmd_buf_alloc_info).front();

    // ------- RECORD COMMAND BUFFER --------

    // algorithm_->record_bind_core(immediate_command_buffer_);

    // algorithm_->record_dispatch(immediate_command_buffer_);

    // constexpr auto begin_info = vk::CommandBufferBeginInfo().setFlags(
    //     vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    // immediate_command_buffer_.begin(begin_info);

    // immediate_command_buffer_.bindPipeline(vk::PipelineBindPoint::eCompute,
    //                                        pipeline_);
    // immediate_command_buffer_.bindDescriptorSets(
    //     vk::PipelineBindPoint::eCompute, pipeline_layout_, 0,
    //     descriptor_set_, nullptr);
    // // push const
    // constexpr uint32_t default_push[3]{0, 0, 0};
    // immediate_command_buffer_.pushConstants(
    //     pipeline_layout_, vk::ShaderStageFlagBits::eCompute, 0,
    //     sizeof(default_push), default_push);

    // constexpr MyPushConst push_const{InputSize(), 0.0f, 1024.0f};
    // immediate_command_buffer_.pushConstants(
    //     pipeline_layout_, vk::ShaderStageFlagBits::eCompute, 16,
    //     sizeof(push_const), &push_const);

    // // equivalent to CUDA number of blocks
    // constexpr auto group_count_x = NumWorkGroup(InputSize());

    // immediate_command_buffer_.dispatch(group_count_x, 1, 1);
    // immediate_command_buffer_.end();

    // ------- SUBMIT COMMAND BUFFER --------

    submit_and_wait(immediate_command_buffer_);
  }

protected:
  void create_sync_object() {
    immediate_fence_ = vkh_device_.createFence(vk::FenceCreateInfo());
  }

  void create_command_pool() {
    const auto create_info =
        vk::CommandPoolCreateInfo()
            .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
            .setQueueFamilyIndex(
                device_.get_queue_index(vkb::QueueType::compute).value());

    command_pool_ = vkh_device_.createCommandPool(create_info);

    const auto command_buffer_allocate_info =
        vk::CommandBufferAllocateInfo()
            .setCommandBufferCount(1)
            .setLevel(vk::CommandBufferLevel::ePrimary)
            .setCommandPool(command_pool_);

    immediate_command_buffer_ =
        vkh_device_.allocateCommandBuffers(command_buffer_allocate_info)
            .front();
  }

private:
  vk::Device vkh_device_;

  vk::CommandPool command_pool_;
  vk::CommandBuffer immediate_command_buffer_;
  vk::Fence immediate_fence_;

  std::vector<std::weak_ptr<YxAlgorithm>> yx_algorithms_;
  std::vector<std::weak_ptr<Buffer>> yx_buffers_;

  // Should the engine manage the above resources?
  bool manage_resources_ = true;

public:
  // std::shared_ptr<DescriptorAllocator> m_descriptorAllocator;
  // std::shared_ptr<DescriptorLayoutCache> m_descriptorLayoutCache;
};
} // namespace core
