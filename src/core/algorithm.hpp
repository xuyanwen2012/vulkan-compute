#pragma once

#include "buffer.hpp"
#include "compute_shader.hpp"
#include <array>
#include <memory>
#include <vulkan/vulkan.hpp>

namespace core {

using Workgroup = std::array<uint32_t, 3>;

// Abstraction of a GPU Computation (kernel function), which is a Shader Module
// and a Pipeline
class Algorithm {
public:
  Algorithm(std::shared_ptr<vk::Device> device,
            // const std::vector<uint32_t> &spirv = {},
            const Workgroup &workgroup = {},
            const std::vector<uint32_t> &specializationConstants = {}) {
    rebuild(workgroup, specializationConstants);
  }

  ~Algorithm() { destroy(); }

  // Rebuild function to reconstruct algorithm with configuration parameters
  //  * to create the underlying resources
  void rebuild(
      // const std::vector<uint32_t> &spirv = {},
      const Workgroup &workgroup = {},
      const std::vector<uint32_t> &specializationConstants = {}) {

    if (specializationConstants.size()) {
      if (this->mSpecializationConstantsData) {
        free(this->mSpecializationConstantsData);
      }
      uint32_t memorySize = sizeof(decltype(specializationConstants.back()));
      uint32_t size = specializationConstants.size();
      uint32_t totalSize = size * memorySize;
      this->mSpecializationConstantsData = malloc(totalSize);
      memcpy(this->mSpecializationConstantsData, specializationConstants.data(),
             totalSize);
      this->mSpecializationConstantsDataTypeMemorySize = memorySize;
      this->mSpecializationConstantsSize = size;
    }

    setWorkgroup(workgroup);

    createParameters();
    createShaderModule();
    createPipeline();
  }

  void recordDispatch(const vk::CommandBuffer &commandBuffer) {}

  // binding pipeline etc
  void recordBindCore(const vk::CommandBuffer &commandBuffer) {
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline_);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                                     pipeline_layout_,
                                     0, // First set
                                     descriptor_set_,
                                     nullptr // Dispatcher
    );
  }

  const Workgroup &getWorkgroup() { return mWorkgroup; }

  void setWorkgroup(const Workgroup &workgroup) { mWorkgroup = workgroup; }

  //  void recordDispatch(const vk::CommandBuffer& commandBuffer);
protected:
  void createShaderModule() {
    compute_module_ = std::make_shared<ComputeShader>(device_ptr_, "None");
  }

  void createPipeline() {
    const auto push_const =
        vk::PushConstantRange()
            .setStageFlags(vk::ShaderStageFlagBits::eCompute)
            .setOffset(0)
            .setSize(this->mPushConstantsDataTypeMemorySize *
                     this->mPushConstantsSize);

    // Pipleline layout (2/3)
    const auto layout_create_info = vk::PipelineLayoutCreateInfo()
                                        .setSetLayoutCount(1)
                                        .setSetLayouts(descriptor_set_layout_)
                                        .setPushConstantRangeCount(1)
                                        .setPushConstantRanges(push_const);
    pipeline_layout_ = device_ptr_->createPipelineLayout(layout_create_info);

    // Pipeline cache (2.5/3)
    const auto pipelineCacheInfo = vk::PipelineCacheCreateInfo();
    pipeline_cache_ = device_ptr_->createPipelineCache(pipelineCacheInfo);

    // Pipeline itself (3/3)
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

    constexpr std::array spec_map_content{1u, 1u, 1u};
    const auto specialization_info =
        vk::SpecializationInfo().setMapEntries(spec_map).setData<uint32_t>(
            spec_map_content);

    const auto shader_stage_create_info =
        vk::PipelineShaderStageCreateInfo()
            .setStage(vk::ShaderStageFlagBits::eCompute)
            .setModule(compute_module_->get_handle())
            .setPName("foo")
            .setPSpecializationInfo(&specialization_info);

    const auto pipeline_create_info = vk::ComputePipelineCreateInfo()
                                          .setStage(shader_stage_create_info)
                                          .setLayout(pipeline_layout_);
    if (auto pipelineResult = device_ptr_->createComputePipeline(
            pipeline_cache_, pipeline_create_info);
        pipelineResult.result != vk::Result::eSuccess) {
      throw std::runtime_error("Cannot create compute pipeline");
    }
  }

  void createParameters() {
    // How many parameters in the shader? usually 2: input and output
    constexpr auto num_parameters = 2u;

    // Pool size
    std::vector pool_sizes{vk::DescriptorPoolSize(
        vk::DescriptorType::eStorageBuffer, num_parameters)};

    // Descriptor pool
    const auto descriptor_pool_create_info =
        vk::DescriptorPoolCreateInfo().setMaxSets(1).setPoolSizes(pool_sizes);
    descriptor_pool_ =
        device_ptr_->createDescriptorPool(descriptor_pool_create_info);

    // Descriptor set
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    for (auto i = 0; i < num_parameters; ++i) {
      bindings.emplace_back(i, // Binding index
                            vk::DescriptorType::eStorageBuffer,
                            1, // Descriptor count
                            vk::ShaderStageFlagBits::eCompute);
    }
    const auto layout_create_info =
        vk::DescriptorSetLayoutCreateInfo().setBindings(bindings);
    descriptor_set_layout_ =
        device_ptr_->createDescriptorSetLayout(layout_create_info, nullptr);

    // Allocate descriptor set
    const auto set_alloc_info = vk::DescriptorSetAllocateInfo()
                                    .setDescriptorPool(descriptor_pool_)
                                    .setDescriptorSetCount(1)
                                    .setSetLayouts(descriptor_set_layout_);
    descriptor_set_ =
        device_ptr_->allocateDescriptorSets(set_alloc_info).front();

    // TODO: -------------------- tmp --------------
    std::array<Buffer, num_parameters> tmp_buffer{Buffer(device_ptr_, 1024),
                                                  Buffer(device_ptr_, 1024)};

    auto tmp_buffer_infos = std::array{
        vk::DescriptorBufferInfo()
            .setBuffer(tmp_buffer[0].get_handle())
            .setOffset(0)
            .setRange(tmp_buffer[0].get_size()),
        vk::DescriptorBufferInfo()
            .setBuffer(tmp_buffer[1].get_handle())
            .setOffset(0)
            .setRange(tmp_buffer[1].get_size()),
    };

    // TODO: -------------------- tmp --------------

    for (auto i = 0; i < num_parameters; ++i) {
      // vk::DescriptorBufferInfo descriptorBufferInfo =
      //   this->mTensors[i]->constructDescriptorBufferInfo();
      const auto &buf_info = tmp_buffer_infos[i];

      std::array computeWriteDescriptorSets{
          vk::WriteDescriptorSet()
              .setDstSet(descriptor_set_)
              .setDstBinding(i)
              .setDstArrayElement(0)
              .setDescriptorCount(1)
              .setDescriptorType(vk::DescriptorType::eStorageBuffer)
              .setBufferInfo(buf_info)};

      device_ptr_->updateDescriptorSets(computeWriteDescriptorSets, nullptr);
    }
  }

  void destroy() {
    device_ptr_->destroyPipeline(pipeline_);
    device_ptr_->destroyPipelineCache(pipeline_cache_);
    device_ptr_->destroyPipelineLayout(pipeline_layout_);
    device_ptr_->destroyDescriptorPool(descriptor_pool_);
    device_ptr_->destroyDescriptorSetLayout(descriptor_set_layout_);
  }

private:
  // -------------- NEVER OWNED RESOURCES
  std::shared_ptr<vk::Device> device_ptr_;
  // vector of tensors (parameters: array of data buffers))

  // -------------- OPTIONALLY OWNED RESOURCES
  std::shared_ptr<ComputeShader> compute_module_;
  vk::Pipeline pipeline_;
  vk::PipelineCache pipeline_cache_;
  vk::PipelineLayout pipeline_layout_;
  vk::DescriptorSetLayout descriptor_set_layout_;
  vk::DescriptorPool descriptor_pool_;
  vk::DescriptorSet descriptor_set_;

  //   std::shared_ptr<vk::DescriptorSetLayout> mDescriptorSetLayout;
  //   std::shared_ptr<vk::DescriptorPool> mDescriptorPool;
  //   std::shared_ptr<vk::DescriptorSet> mDescriptorSet;
  //   std::shared_ptr<vk::ShaderModule> mShaderModule;
  //   std::shared_ptr<vk::PipelineLayout> mPipelineLayout;
  //   std::shared_ptr<vk::PipelineCache> mPipelineCache;
  //   std::shared_ptr<vk::Pipeline> mPipeline;

  // -------------- ALWAYS OWNED RESOURCES

  //   std::vector<uint32_t> spirv_;
  void *mSpecializationConstantsData = nullptr;
  uint32_t mSpecializationConstantsDataTypeMemorySize = 0;
  uint32_t mSpecializationConstantsSize = 0;
  void *mPushConstantsData = nullptr;
  uint32_t mPushConstantsDataTypeMemorySize = 0;
  uint32_t mPushConstantsSize = 0;
  Workgroup mWorkgroup;
};

} // namespace core