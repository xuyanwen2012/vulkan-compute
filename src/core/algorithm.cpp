#include "algorithm.hpp"

#include <cstdint>

#include "../common/shader_loader.hpp"

// For CLSPV generated shader, need to use specialization constants to pass
// Must need 3 uint32 at constant ID 0, 1, 2. The vaule of them is the workgroup
// size. i.e., the number of threads you want the shader to launch with.
//
// I am hard coding the first three specialized entry right now.
[[nodiscard]] constexpr auto clspv_default_spec_const() {
  std::array<vk::SpecializationMapEntry, 3> spec_const;
  for (int i = 0; i < 3; ++i) {
    spec_const[i] = vk::SpecializationMapEntry()
                        .setConstantID(i)
                        .setOffset(i * sizeof(uint32_t))
                        .setSize(sizeof(uint32_t));
  }
  return spec_const;
}

namespace core {

Algorithm::Algorithm(std::shared_ptr<vk::Device> device_ptr,
                     const std::string_view spirv_filename,
                     const std::vector<std::shared_ptr<Buffer>> &buffers,
                     const uint32_t threads_per_block,
                     const std::vector<float> &push_constants)
    : VulkanResource(std::move(device_ptr)),
      spirv_filename_(spirv_filename),
      usm_buffers_(buffers),
      threads_per_block_(threads_per_block) {
  spdlog::info("YxAlgorithm ({}) initializing with number of buffers: {}",
               spirv_filename,
               buffers.size());

  set_push_constants(push_constants);

  create_shader_module();
  create_parameters();
  create_pipeline();
}

void Algorithm::destroy() {
  spdlog::debug("YxAlgorithm::destroy");
  device_ptr_->destroyShaderModule(handle_);
  device_ptr_->destroyPipeline(pipeline_);
  device_ptr_->destroyPipelineCache(pipeline_cache_);
  device_ptr_->destroyPipelineLayout(pipeline_layout_);
  device_ptr_->destroyDescriptorSetLayout(descriptor_set_layout_);
  device_ptr_->destroyDescriptorPool(descriptor_pool_);
}

void Algorithm::set_push_constants(const void *data,
                                   const uint32_t size,
                                   const uint32_t memory_size) {
  const uint32_t total_size = size * memory_size;

  push_constants_data_ = malloc(total_size);
  std::memcpy(push_constants_data_, data, total_size);

  push_constants_data_type_memory_size_ = memory_size;
  push_constants_size_ = size;
}

void Algorithm::record_bind_core(const vk::CommandBuffer &cmd_buf) const {
  cmd_buf.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline_);
  cmd_buf.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                             pipeline_layout_,
                             0,
                             descriptor_set_,
                             nullptr);
}

void Algorithm::record_bind_push(const vk::CommandBuffer &cmd_buf) const {
  spdlog::debug("YxAlgorithm::record_bind_push, constants memory size: {}",
                push_constants_size_ * push_constants_data_type_memory_size_);

  cmd_buf.pushConstants(
      pipeline_layout_,
      vk::ShaderStageFlagBits::eCompute,
      0,
      push_constants_size_ * push_constants_data_type_memory_size_,
      push_constants_data_);
}

void Algorithm::record_dispatch_tmp(const vk::CommandBuffer &cmd_buf,
                                    const uint32_t data_size) const {
  const auto num_blocks =
      (data_size + threads_per_block_ - 1u) / threads_per_block_;
  spdlog::info("YxAlgorithm::record_dispatch_tmp, num_blocks: {}", num_blocks);
  cmd_buf.dispatch(num_blocks, 1u, 1u);
}

void Algorithm::create_parameters() {
  // Pool size
  std::vector pool_sizes{vk::DescriptorPoolSize(
      vk::DescriptorType::eStorageBuffer, usm_buffers_.size())};

  // Descriptor pool
  const auto descriptor_pool_create_info =
      vk::DescriptorPoolCreateInfo().setMaxSets(1).setPoolSizes(pool_sizes);
  descriptor_pool_ =
      device_ptr_->createDescriptorPool(descriptor_pool_create_info);

  // Descriptor set
  std::vector<vk::DescriptorSetLayoutBinding> bindings;
  bindings.reserve(usm_buffers_.size());
  for (auto i = 0u; i < usm_buffers_.size(); ++i) {
    bindings.emplace_back(i,  // Binding index
                          vk::DescriptorType::eStorageBuffer,
                          1,  // Descriptor count
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
  descriptor_set_ = device_ptr_->allocateDescriptorSets(set_alloc_info).front();

  // Update descriptor set
  for (auto i = 0u; i < usm_buffers_.size(); ++i) {
    std::vector<vk::WriteDescriptorSet> compute_write_descriptor_sets;
    compute_write_descriptor_sets.reserve(usm_buffers_.size());

    const vk::DescriptorBufferInfo buf_info =
        usm_buffers_[i]->construct_descriptor_buffer_info();

    compute_write_descriptor_sets.emplace_back(
        descriptor_set_,
        i,  // Destination binding
        0,  // Destination array element
        1,  // Descriptor count
        vk::DescriptorType::eStorageBuffer,
        nullptr,  // Descriptor image info
        &buf_info);

    device_ptr_->updateDescriptorSets(compute_write_descriptor_sets, nullptr);
  }
}

void Algorithm::create_pipeline() {
  // Push constant Range (1.5/3)
  const auto push_const = vk::PushConstantRange()
                              .setStageFlags(vk::ShaderStageFlagBits::eCompute)
                              .setOffset(0)
                              .setSize(push_constants_data_type_memory_size_ *
                                       push_constants_size_);

  // Pipeline layout (2/3)
  const auto layout_create_info = vk::PipelineLayoutCreateInfo()
                                      .setSetLayoutCount(1)
                                      .setSetLayouts(descriptor_set_layout_)
                                      .setPushConstantRangeCount(1)
                                      .setPushConstantRanges(push_const);

  pipeline_layout_ = device_ptr_->createPipelineLayout(layout_create_info);

  // Pipeline cache (2.5/3)
  constexpr auto pipeline_cache_info = vk::PipelineCacheCreateInfo();
  pipeline_cache_ = device_ptr_->createPipelineCache(pipeline_cache_info);

  // Specialization info (2.75/3) telling the shader the workgroup size
  const std::array spec_map_content{threads_per_block_, 1u, 1u};

  const auto spec_map = clspv_default_spec_const();
  const auto spec_info = vk::SpecializationInfo()
                             .setMapEntries(spec_map)  // 3 entries, = workgroup
                             .setData<uint32_t>(spec_map_content);

  // Pipeline itself (3/3)
  const auto shader_stage_create_info =
      vk::PipelineShaderStageCreateInfo()
          .setStage(vk::ShaderStageFlagBits::eCompute)
          .setModule(handle_)
          .setPName("foo")
          .setPSpecializationInfo(&spec_info);

  const auto create_info = vk::ComputePipelineCreateInfo()
                               .setStage(shader_stage_create_info)
                               .setLayout(pipeline_layout_);

  pipeline_ =
      device_ptr_->createComputePipeline(pipeline_cache_, create_info).value;
}

void Algorithm::create_shader_module() {
  const auto spirv_binary = load_shader_from_file(spirv_filename_);

  const auto create_info = vk::ShaderModuleCreateInfo().setCode(spirv_binary);
  handle_ = device_ptr_->createShaderModule(create_info);
}

}  // namespace core
