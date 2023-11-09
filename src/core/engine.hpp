#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

#include "base_engine.hpp"
#include "buffer.hpp"
#include "compute_shader.hpp"

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
    // Tmp
    vkh_device_ = device_.device;

    create_descriptor_set_layout();
    create_descriptor_pool();
    
    create_sync_object();

    // Tmp
    usm_buffers_.emplace_back(std::make_shared<Buffer>(
        get_device_ptr(), InputSize() * sizeof(InputT)));
    usm_buffers_.emplace_back(std::make_shared<Buffer>(
        get_device_ptr(), InputSize() * sizeof(OutputT)));

    create_descriptor_set();
    create_compute_pipeline();

    create_command_pool();
  }

  ~ComputeEngine() {
    vkh_device_.destroyFence(immediate_fence_);
    vkh_device_.freeCommandBuffers(command_pool_, immediate_command_buffer_);
    vkh_device_.destroyCommandPool(command_pool_);
    vkh_device_.destroyDescriptorSetLayout(descriptor_set_layout_);
    vkh_device_.destroyDescriptorPool(descriptor_pool_);
    vkh_device_.destroyPipelineLayout(pipeline_layout_);
    vkh_device_.destroyPipeline(pipeline_);
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
    // write_data_to_buffer
    usm_buffers_[0]->update(input_data.data(), sizeof(InputT) * InputSize());

    // execute_sync
    const auto cmd_buf_alloc_info =
        vk::CommandBufferAllocateInfo()
            .setCommandPool(command_pool_)
            .setLevel(vk::CommandBufferLevel::ePrimary)
            .setCommandBufferCount(1);

    immediate_command_buffer_ =
        vkh_device_.allocateCommandBuffers(cmd_buf_alloc_info).front();

    // ------- RECORD COMMAND BUFFER --------
    constexpr auto begin_info = vk::CommandBufferBeginInfo().setFlags(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    immediate_command_buffer_.begin(begin_info);

    immediate_command_buffer_.bindPipeline(vk::PipelineBindPoint::eCompute,
                                           pipeline_);
    immediate_command_buffer_.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute, pipeline_layout_, 0, descriptor_set_,
        nullptr);
    // push const
    constexpr uint32_t default_push[3]{0, 0, 0};
    immediate_command_buffer_.pushConstants(
        pipeline_layout_, vk::ShaderStageFlagBits::eCompute, 0,
        sizeof(default_push), default_push);

    constexpr MyPushConst push_const{InputSize(), 0.0f, 1024.0f};
    immediate_command_buffer_.pushConstants(
        pipeline_layout_, vk::ShaderStageFlagBits::eCompute, 16,
        sizeof(push_const), &push_const);

    // equivalent to CUDA number of blocks
    constexpr auto group_count_x = NumWorkGroup(InputSize());

    immediate_command_buffer_.dispatch(group_count_x, 1, 1);
    immediate_command_buffer_.end();

    // ------- SUBMIT COMMAND BUFFER --------

    submit_and_wait(immediate_command_buffer_);
  }

protected:
  void create_descriptor_set_layout() {
    constexpr std::array bindings{
        vk::DescriptorSetLayoutBinding()
            .setBinding(0)
            .setDescriptorType(vk::DescriptorType::eStorageBuffer)
            .setDescriptorCount(1)
            .setStageFlags(vk::ShaderStageFlagBits::eCompute),
        vk::DescriptorSetLayoutBinding()
            .setBinding(1)
            .setDescriptorType(vk::DescriptorType::eStorageBuffer)
            .setDescriptorCount(1)
            .setStageFlags(vk::ShaderStageFlagBits::eCompute)};

    const auto layout_create_info =
        vk::DescriptorSetLayoutCreateInfo().setBindings(bindings);
    descriptor_set_layout_ =
        vkh_device_.createDescriptorSetLayout(layout_create_info, nullptr);
  }

  void create_sync_object() {
    immediate_fence_ = vkh_device_.createFence(vk::FenceCreateInfo());
  }

  void create_descriptor_pool() {
    std::vector pool_sizes{vk::DescriptorPoolSize()
                               .setType(vk::DescriptorType::eStorageBuffer)
                               .setDescriptorCount(2)};

    const auto descriptor_pool_create_info =
        vk::DescriptorPoolCreateInfo().setMaxSets(1).setPoolSizes(pool_sizes);

    descriptor_pool_ =
        vkh_device_.createDescriptorPool(descriptor_pool_create_info);
  }

  void create_descriptor_set() {
    const auto set_alloc_info = vk::DescriptorSetAllocateInfo()
                                    .setDescriptorPool(descriptor_pool_)
                                    .setDescriptorSetCount(1)
                                    .setSetLayouts(descriptor_set_layout_);

    descriptor_set_ =
        vkh_device_.allocateDescriptorSets(set_alloc_info).front();

    const auto in_buffer_info = vk::DescriptorBufferInfo()
                                    .setBuffer(usm_buffers_[0]->get_handle())
                                    .setOffset(0)
                                    .setRange(usm_buffers_[0]->get_size());

    const auto out_buffer_info = vk::DescriptorBufferInfo()
                                     .setBuffer(usm_buffers_[1]->get_handle())
                                     .setOffset(0)
                                     .setRange(usm_buffers_[1]->get_size());

    const std::array writes{
        vk::WriteDescriptorSet()
            .setDstSet(descriptor_set_)
            .setDstBinding(0)
            .setDstArrayElement(0)
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eStorageBuffer)
            .setBufferInfo(in_buffer_info),
        vk::WriteDescriptorSet()
            .setDstSet(descriptor_set_)
            .setDstBinding(1)
            .setDstArrayElement(0)
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eStorageBuffer)
            .setBufferInfo(out_buffer_info),
    };

    vkh_device_.updateDescriptorSets(writes.size(), writes.data(), 0, nullptr);
  }

  void create_compute_pipeline() {
    ComputeShader compute_module{get_device_ptr(), "None"};

    // For CLSPV generated shader, need to use specialization constants to pass
    // the number of "ThreadsPerBlock" (CUDA term)
    std::array spec_map{
        vk::SpecializationMapEntry().setConstantID(0).setOffset(0).setSize(
            sizeof(uint32_t)),
        vk::SpecializationMapEntry().setConstantID(1).setOffset(4).setSize(
            sizeof(uint32_t)),
        vk::SpecializationMapEntry().setConstantID(2).setOffset(8).setSize(
            sizeof(uint32_t)),
    };

    constexpr std::array spec_map_content{ComputeShaderProcessUnit(), 1u, 1u};
    const auto specialization_info =
        vk::SpecializationInfo()
            .setMapEntries(spec_map)
            //.setData(spec_map_content)
            .setDataSize(sizeof(uint32_t) * spec_map_content.size())
            .setPData(spec_map_content.data());

    const auto shader_stage_create_info =
        vk::PipelineShaderStageCreateInfo()
            .setStage(vk::ShaderStageFlagBits::eCompute)
            .setModule(compute_module.get_handle())
            .setPName("foo")
            .setPSpecializationInfo(&specialization_info);

    // For CLSPV generated shaders, need to push 16 bytes of place holder data,
    // before pushing the actual constants used by the kernel function
    constexpr auto push_const =
        vk::PushConstantRange()
            .setStageFlags(vk::ShaderStageFlagBits::eCompute)
            .setOffset(0)
            .setSize(16 + sizeof(MyPushConst));

    // Create a Pipeline Layout (2/3)
    const auto layout_create_info = vk::PipelineLayoutCreateInfo()
                                        .setSetLayoutCount(1)
                                        .setSetLayouts(descriptor_set_layout_)
                                        .setPushConstantRangeCount(1)
                                        .setPushConstantRanges(push_const);

    pipeline_layout_ = vkh_device_.createPipelineLayout(layout_create_info);

    // Pipeline itself (3/3)
    const auto pipeline_create_info = vk::ComputePipelineCreateInfo()
                                          .setStage(shader_stage_create_info)
                                          .setLayout(pipeline_layout_);

    auto result =
        vkh_device_.createComputePipeline(nullptr, pipeline_create_info);
    if (result.result != vk::Result::eSuccess) {
      throw std::runtime_error("Cannot create compute pipeline");
    }

    pipeline_ = result.value; // pipeline_layout
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

  // Only need one for entire application,
  // other things are: instance, physical device, device, queue
  // and global VMA allocator
  vk::CommandPool command_pool_;

  vk::CommandBuffer immediate_command_buffer_;
  vk::Fence immediate_fence_;

  // These are for each compute shader (pipeline)
  vk::Pipeline pipeline_;
  vk::PipelineCache pipeline_cache_;
  vk::PipelineLayout pipeline_layout_;
  vk::DescriptorSetLayout descriptor_set_layout_;
  vk::DescriptorPool descriptor_pool_;
  vk::DescriptorSet descriptor_set_;
  // and module

public:
  // Warning: use pointer, other wise the buffer's content might be gone
  std::vector<std::shared_ptr<Buffer>> usm_buffers_;

  // std::shared_ptr<DescriptorAllocator> m_descriptorAllocator;
  // std::shared_ptr<DescriptorLayoutCache> m_descriptorLayoutCache;
};
} // namespace core
