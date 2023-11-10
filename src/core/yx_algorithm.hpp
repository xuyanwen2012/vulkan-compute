#pragma once

#include "vulkan_resource.hpp"
#include <iostream>

namespace core {

class YxAlgorithm final : public VulkanResource<vk::ShaderModule> {

public:
  YxAlgorithm(const std::shared_ptr<vk::Device> &device_ptr)
      : VulkanResource(device_ptr) {}

  void destroy() override {
    device_ptr_->destroyShaderModule(handle_);
    device_ptr_->destroyPipeline(pipeline_);
    device_ptr_->destroyPipelineCache(pipeline_cache_);
    device_ptr_->destroyPipelineLayout(pipeline_layout_);
    device_ptr_->destroyDescriptorSetLayout(descriptor_set_layout_);
    device_ptr_->destroyDescriptorPool(descriptor_pool_);
  }

  template <typename T>
  void set_push_constants(const std::vector<T> &push_constants) {
    assert(!push_constants.empty());
    const uint32_t memory_size = sizeof(decltype(push_constants.back()));
    const uint32_t size = push_constants.size();
    assert(memory_size > 0);
    assert(size > 0);
    this->set_push_constants(push_constants.data(), size, memory_size);
  }

  void set_push_constants(const void *data, const uint32_t size,
                          const uint32_t memory_size) {
    const uint32_t total_size = memory_size * size;

    if (const uint32_t previous_total_size =
            push_constants_size_ * push_constants_data_type_memory_size_;
        total_size != previous_total_size) {

      // throw std::runtime_error("totalSize != previousTotalSize");
      std::cout << "[Warning] totalSize != previousTotalSize" << std::endl;
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

protected:
  void create_parameters();
  void create_pipeline();
  void create_shader_module();

private:
  vk::Pipeline pipeline_;
  vk::PipelineCache pipeline_cache_;
  vk::PipelineLayout pipeline_layout_;
  vk::DescriptorSetLayout descriptor_set_layout_;
  vk::DescriptorPool descriptor_pool_;
  vk::DescriptorSet descriptor_set_;

  std::array<uint32_t, 3> workgroup_;

  void *specialization_constants_data_ = nullptr;
  uint32_t specialization_constants_data_type_memory_size_ = 0;
  uint32_t specialization_constants_size_ = 0;

  void *push_constants_data_ = nullptr;
  uint32_t push_constants_data_type_memory_size_ = 0;
  uint32_t push_constants_size_ = 0;
};

} // namespace core