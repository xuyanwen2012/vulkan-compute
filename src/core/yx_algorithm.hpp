#pragma once

#include "../../shaders/all_shaders.hpp"
#include "buffer.hpp"
#include "vulkan_resource.hpp"
#include <spdlog/spdlog.h>
namespace core {

// For CLSPV generated shader, need to use specialization constants to pass
// Must need 3 uint32 at constant ID 0, 1, 2
// I am hard coding the first three specialized entry right now
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

class YxAlgorithm final : public VulkanResource<vk::ShaderModule> {

public:
  YxAlgorithm(const std::shared_ptr<vk::Device> &device_ptr,
              const std::string_view spirv_filename,
              const std::vector<std::shared_ptr<Buffer>> &buffers = {},
              const std::array<uint32_t, 3> &workgroup = {},
              const std::vector<float> &push_constants = {})
      : VulkanResource(device_ptr), spirv_filename_(spirv_filename),
        usm_buffers_(buffers) {
    if (!buffers.empty()) {
      spdlog::info("YxAlgorithm ({}) initializing with tensor size: {}",
                   spirv_filename, buffers.size());
      rebuild(workgroup, push_constants);
    } else {
      spdlog::error("YxAlgorithm ({}) initializing with empty tensor",
                    spirv_filename);
    }
  }

  ~YxAlgorithm() override {
    spdlog::debug("YxAlgorithm::~YxAlgorithm");
    destroy();
  }

  void rebuild(const std::array<uint32_t, 3> &workgroup,
               const std::vector<float> &push_constants) {
    spdlog::debug("YxAlgorithm::rebuild, workgroup_x: {}, pushConstants: {}",
                  workgroup[0], push_constants.size());

    if (!push_constants.empty()) {
      if (push_constants_data_) {
        free(push_constants_data_);
      }
      constexpr uint32_t memory_size = sizeof(decltype(push_constants.back()));
      const uint32_t size = push_constants.size();
      const uint32_t total_size = size * memory_size;
      push_constants_data_ = malloc(total_size);
      std::memcpy(push_constants_data_, push_constants.data(), total_size);
      push_constants_data_type_memory_size_ = memory_size;
      push_constants_size_ = size;
    }

    set_workgroup(workgroup);

    create_shader_module();
    create_parameters();
    create_pipeline();
  }

  void destroy() override {
    spdlog::debug("YxAlgorithm::destroy");
    device_ptr_->destroyShaderModule(handle_);
    device_ptr_->destroyPipeline(pipeline_);
    device_ptr_->destroyPipelineCache(pipeline_cache_);
    device_ptr_->destroyPipelineLayout(pipeline_layout_);
    device_ptr_->destroyDescriptorSetLayout(descriptor_set_layout_);
    device_ptr_->destroyDescriptorPool(descriptor_pool_);
  }

  template <typename T>
  void set_push_constants(const std::vector<T> &push_constants) {
    spdlog::debug("YxAlgorithm::set_push_constants", push_constants.size());
    const uint32_t memory_size = sizeof(decltype(push_constants.back()));
    const uint32_t size = push_constants.size();
    this->set_push_constants(push_constants.data(), size, memory_size);
  }

  void set_push_constants(const void *data, const uint32_t size,
                          const uint32_t memory_size) {
    const uint32_t total_size = memory_size * size;

    if (const uint32_t previous_total_size =
            push_constants_size_ * push_constants_data_type_memory_size_;
        total_size != previous_total_size) {
      spdlog::warn("totalSize != previousTotalSize");
    }

    if (push_constants_data_) {
      free(push_constants_data_);
    }

    push_constants_data_ = malloc(total_size);
    memcpy(push_constants_data_, data, total_size);
    push_constants_data_type_memory_size_ = memory_size;
    push_constants_size_ = size;
  }

  void set_workgroup(const std::array<uint32_t, 3> &workgroup) {
    workgroup_ = workgroup;
  }

  template <typename T> std::vector<T> get_push_constants() {
    return {static_cast<T *>(push_constants_data_),
            static_cast<T *>(push_constants_data_) + push_constants_size_};
  }

protected:
  // Basically setup the buffer, its descriptor set, binding etc.
  void create_parameters() {
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

    // Update descriptor set
    for (auto i = 0u; i < usm_buffers_.size(); ++i) {
      std::vector<vk::WriteDescriptorSet> compute_write_descriptor_sets;
      compute_write_descriptor_sets.reserve(usm_buffers_.size());

      vk::DescriptorBufferInfo buf_info =
          usm_buffers_[i]->constructDescriptorBufferInfo();

      compute_write_descriptor_sets.emplace_back(
          descriptor_set_,
          i, // Destination binding
          0, // Destination array element
          1, // Descriptor count
          vk::DescriptorType::eStorageBuffer,
          nullptr, // Descriptor image info
          &buf_info);

      device_ptr_->updateDescriptorSets(compute_write_descriptor_sets, nullptr);
    }
  }

  void create_pipeline() {
    // Push constant Range (1.5/3)
    const auto push_const =
        vk::PushConstantRange()
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

    // Pipeline itself (3/3)
    const auto spec_map = clspv_default_spec_const();
    const std::array spec_map_content{workgroup_[0], workgroup_[1],
                                      workgroup_[2]};

    const auto spec_info =
        vk::SpecializationInfo()
            .setMapEntries(spec_map) // 3 entries, = workgroup
            .setData<uint32_t>(spec_map_content);

    const auto shader_stage_create_info =
        vk::PipelineShaderStageCreateInfo()
            .setStage(vk::ShaderStageFlagBits::eCompute)
            .setModule(handle_)
            .setPName("foo")
            .setPSpecializationInfo(&spec_info);

    const auto create_info = vk::ComputePipelineCreateInfo()
                                 .setStage(shader_stage_create_info)
                                 .setLayout(pipeline_layout_);

#ifdef CREATE_PIPELINE_RESULT_VALUE
    const auto pipeline_result =
        device_ptr_->createComputePipeline(pipeline_cache_, create_info);
    if (pipeline_result.result != vk::Result::eSuccess) {
      throw std::runtime_error("Cannot create compute pipeline");
    }
    pipeline_ = pipeline_result.value;
#else
    pipeline_ =
        device_ptr_->createComputePipeline(pipeline_cache_, create_info).value;
#endif
  }

  void create_shader_module() {
    //  ------
    auto &shader_code = morton_spv;
    // ------

    const std::vector spivr_binary(shader_code,
                                   shader_code + std::size(shader_code));

    const auto create_info = vk::ShaderModuleCreateInfo().setCode(spivr_binary);
    handle_ = device_ptr_->createShaderModule(create_info);
  }

private:
  std::string spirv_filename_;

  vk::Pipeline pipeline_;
  vk::PipelineCache pipeline_cache_;
  vk::PipelineLayout pipeline_layout_;
  vk::DescriptorSetLayout descriptor_set_layout_;
  vk::DescriptorPool descriptor_pool_;
  vk::DescriptorSet descriptor_set_;

  std::array<uint32_t, 3> workgroup_;

  std::vector<std::shared_ptr<Buffer>> usm_buffers_;

  void *push_constants_data_ = nullptr;
  uint32_t push_constants_data_type_memory_size_ = 0;
  uint32_t push_constants_size_ = 0;
};

} // namespace core