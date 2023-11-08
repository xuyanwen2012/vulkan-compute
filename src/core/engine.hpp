#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

#include "base_engine.hpp"
#include "buffer.hpp"
#include "compute_shader.hpp"

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

struct MyPushConsts {
  uint32_t n;
  float min_coord;
  float range;
};

namespace core {

class ComputeEngine : public BaseEngine {
public:
  ComputeEngine() : BaseEngine() {
    // Tmp
    vkhDevice = device.device;
    vkhQueue = queue;

    // Create instance, device, etc. Done in BaseEngine

    create_descriptor_set_layout();
    create_descriptor_pool();

    create_sync_object();

    // Tmp
    usm_buffers.emplace_back(
        std::make_shared<Buffer>(InputSize() * sizeof(InputT)));
    usm_buffers.emplace_back(
        std::make_shared<Buffer>(InputSize() * sizeof(OutputT)));

    create_descriptor_set();
    create_compute_pipeline();

    create_command_pool();
  }

  ~ComputeEngine() {
    // vkhDevice.waitIdle();

    vkhDevice.destroyFence(m_immediateFence);

    vkhDevice.freeCommandBuffers(command_pool, command_buffer);
    vkhDevice.destroyCommandPool(command_pool);

    vkhDevice.destroyDescriptorSetLayout(descriptor_set_layout);
    vkhDevice.destroyDescriptorPool(descriptor_pool);

    vkhDevice.destroyPipelineLayout(pipeline_layout);
    vkhDevice.destroyPipeline(pipeline);
  }

  void submit(const vk::CommandBuffer &commandBuffer) {
    vk::SubmitInfo submitInfo;
    submitInfo.setCommandBuffers(commandBuffer);

    if (const auto result = disp.queueSubmit(
            queue, 1, reinterpret_cast<VkSubmitInfo *>(&submitInfo),
            VK_NULL_HANDLE);
        result != VK_SUCCESS) {
      throw std::runtime_error("Cannot submit queue");
    }

    auto waitResult =
        vkhDevice.waitForFences(m_immediateFence, false, UINT64_MAX);
    assert(waitResult == vk::Result::eSuccess);
    vkhDevice.resetFences(m_immediateFence);
  }

  void run(const std::vector<InputT> &input_data) {
    // write_data_to_buffer
    usm_buffers[0]->update(input_data.data(), sizeof(InputT) * InputSize());

    // execute_sync
    const auto cmd_buf_alloc_info =
        vk::CommandBufferAllocateInfo()
            .setCommandPool(command_pool)
            .setLevel(vk::CommandBufferLevel::ePrimary)
            .setCommandBufferCount(1);

    command_buffer =
        vkhDevice.allocateCommandBuffers(cmd_buf_alloc_info).front();

    // ------- RECORD COMMAND BUFFER --------
    const auto begin_info = vk::CommandBufferBeginInfo().setFlags(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    command_buffer.begin(begin_info);

    command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline);
    command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                                      pipeline_layout, 0, descriptor_set,
                                      nullptr);
    // push consts
    constexpr uint32_t default_push[3]{0, 0, 0};
    command_buffer.pushConstants(pipeline_layout,
                                 vk::ShaderStageFlagBits::eCompute, 0,
                                 sizeof(default_push), default_push);

    constexpr MyPushConsts push_const{InputSize(), 0.0f, 1024.0f};
    command_buffer.pushConstants(pipeline_layout,
                                 vk::ShaderStageFlagBits::eCompute, 16,
                                 sizeof(push_const), &push_const);

    // equvalent to CUDA's number of blocks
    constexpr auto group_count_x = NumWorkGroup(InputSize());

    command_buffer.dispatch(group_count_x, 1, 1);
    command_buffer.end();

    // ------- SUBMIT COMMAND BUFFER --------

    const auto submit_info = vk::SubmitInfo()
                                 .setCommandBufferCount(1)
                                 // wait semaphores?
                                 .setCommandBuffers(command_buffer);

    vkhQueue.submit(submit_info, m_immediateFence);

    // Wait
    auto waitFenceResult =
        vkhDevice.waitForFences(m_immediateFence, false, UINT64_MAX);
    assert(waitFenceResult == vk::Result::eSuccess);
    vkhDevice.resetFences(m_immediateFence);
  }

protected:
  void create_descriptor_set_layout() {
    std::vector<vk::DescriptorSetLayoutBinding> bindings;

    // Input
    bindings.emplace_back(
        vk::DescriptorSetLayoutBinding()
            .setBinding(0)
            .setDescriptorType(vk::DescriptorType::eStorageBuffer)
            .setDescriptorCount(1)
            .setStageFlags(vk::ShaderStageFlagBits::eCompute));

    // Output
    bindings.emplace_back(
        vk::DescriptorSetLayoutBinding()
            .setBinding(1)
            .setDescriptorType(vk::DescriptorType::eStorageBuffer)
            .setDescriptorCount(1)
            .setStageFlags(vk::ShaderStageFlagBits::eCompute));

    const auto layout_create_info = vk::DescriptorSetLayoutCreateInfo()
                                        .setBindingCount(bindings.size())
                                        .setPBindings(bindings.data());
    descriptor_set_layout =
        vkhDevice.createDescriptorSetLayout(layout_create_info, nullptr);
  }

  void create_sync_object() {
    m_immediateFence = vkhDevice.createFence(vk::FenceCreateInfo());
  }

  void create_descriptor_pool() {
    std::vector<vk::DescriptorPoolSize> pool_sizes;

    //  Compute pipelines uses a storage image for image reads and writes
    pool_sizes.emplace_back(vk::DescriptorPoolSize()
                                .setType(vk::DescriptorType::eStorageBuffer)
                                .setDescriptorCount(2));

    const auto descriptor_pool_create_info =
        vk::DescriptorPoolCreateInfo()
            .setMaxSets(1)
            .setPoolSizeCount(pool_sizes.size())
            .setPPoolSizes(pool_sizes.data());

    descriptor_pool =
        vkhDevice.createDescriptorPool(descriptor_pool_create_info);
  }

  void create_descriptor_set() {
    const auto set_alloc_info = vk::DescriptorSetAllocateInfo()
                                    .setDescriptorPool(descriptor_pool)
                                    .setDescriptorSetCount(1)
                                    .setSetLayouts(descriptor_set_layout);

    descriptor_set = vkhDevice.allocateDescriptorSets(set_alloc_info).front();

    const auto in_buffer_info = vk::DescriptorBufferInfo()
                                    .setBuffer(usm_buffers[0]->get_handle())
                                    .setOffset(0)
                                    .setRange(usm_buffers[0]->get_size());

    const auto out_buffer_info = vk::DescriptorBufferInfo()
                                     .setBuffer(usm_buffers[1]->get_handle())
                                     .setOffset(0)
                                     .setRange(usm_buffers[1]->get_size());

    std::array<vk::WriteDescriptorSet, 2> writes{
        vk::WriteDescriptorSet()
            .setDstSet(descriptor_set)
            .setDstBinding(0)
            .setDstArrayElement(0)
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eStorageBuffer)
            .setBufferInfo(in_buffer_info),
        vk::WriteDescriptorSet()
            .setDstSet(descriptor_set)
            .setDstBinding(1)
            .setDstArrayElement(0)
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eStorageBuffer)
            .setBufferInfo(out_buffer_info),
    };

    vkhDevice.updateDescriptorSets(writes.size(), writes.data(), 0, nullptr);
  }

  void create_compute_pipeline() {
    ComputeShader compute_module{get_device_ptr(), "None"};

    // For CLSPV generated shaders, need to use specalization constants to pass
    // the number of "ThreadsPerBlock" (CUDA term)
    constexpr std::size_t num_of_entries = 3u;
    std::array<vk::SpecializationMapEntry, num_of_entries> spec_map{
        vk::SpecializationMapEntry().setConstantID(0).setOffset(0).setSize(
            sizeof(uint32_t)),
        vk::SpecializationMapEntry().setConstantID(1).setOffset(4).setSize(
            sizeof(uint32_t)),
        vk::SpecializationMapEntry().setConstantID(2).setOffset(8).setSize(
            sizeof(uint32_t)),
    };

    constexpr std::array<uint32_t, num_of_entries> spec_map_content{
        ComputeShaderProcessUnit(), 1, 1};

    const auto specialization_info =
        vk::SpecializationInfo()
            .setMapEntryCount(spec_map.size())
            .setPMapEntries(spec_map.data())
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
            .setSize(16 + sizeof(MyPushConsts));

    // Create a Pipeline Layout (2/3)
    const auto layout_create_info = vk::PipelineLayoutCreateInfo()
                                        .setSetLayoutCount(1)
                                        .setSetLayouts(descriptor_set_layout)
                                        .setPushConstantRangeCount(1)
                                        .setPushConstantRanges(push_const);

    pipeline_layout = vkhDevice.createPipelineLayout(layout_create_info);

    // Pipeline itself (3/3)
    const auto pipeline_create_info = vk::ComputePipelineCreateInfo()
                                          .setStage(shader_stage_create_info)
                                          .setLayout(pipeline_layout);

    auto result =
        vkhDevice.createComputePipeline(nullptr, pipeline_create_info);
    if (result.result != vk::Result::eSuccess) {
      throw std::runtime_error("Cannot create compute pipeline");
    }

    pipeline = result.value; // pipeline_layout
  };

  void create_command_pool() {
    const auto createInfo =
        vk::CommandPoolCreateInfo()
            .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
            .setQueueFamilyIndex(
                device.get_queue_index(vkb::QueueType::compute).value());

    command_pool = vkhDevice.createCommandPool(createInfo);

    const auto commandBufferAllocateInfo =
        vk::CommandBufferAllocateInfo()
            .setCommandBufferCount(1)
            .setLevel(vk::CommandBufferLevel::ePrimary)
            .setCommandPool(command_pool);

    command_buffer =
        vkhDevice.allocateCommandBuffers(commandBufferAllocateInfo).front();
  }

private:
  vk::Device vkhDevice;
  vk::Queue vkhQueue;

  // Only need one for entire application,
  // otherthings are: instance, physical device, device, queue
  // and global VMA allocator
  vk::CommandPool command_pool;
  // std::vector<vk::CommandPool> m_threadCommandPool;

  vk::CommandBuffer command_buffer; // immediate command buffer

  vk::Fence m_immediateFence;
  // These are for each compute shader (pipeline)

  vk::Pipeline pipeline;
  vk::PipelineLayout pipeline_layout;

  vk::DescriptorSetLayout descriptor_set_layout;
  vk::DescriptorPool descriptor_pool;
  vk::DescriptorSet descriptor_set;

  // Warning: use pointer, other wise the buffer's content might be gone
  std::vector<std::shared_ptr<Buffer>> usm_buffers;

  // std::shared_ptr<DescriptorAllocator> m_descriptorAllocator;
  // std::shared_ptr<DescriptorLayoutCache> m_descriptorLayoutCache;
};

} // namespace core