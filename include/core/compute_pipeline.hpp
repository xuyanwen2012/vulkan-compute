#pragma once

#include "compute_shader.hpp"

namespace core {

class ComputePipeline final : public VulkanResource<vk::Pipeline> {
 public:
  explicit ComputePipeline(const std::shared_ptr<vk::Device>& device_ptr)
      : VulkanResource(device_ptr) {
    create_descriptor_pool();
  }

  ~ComputePipeline() override { destroy(); }

  void destroy() override {
    device_ptr_->destroyPipeline(handle_);
    device_ptr_->destroyPipelineCache(pipeline_cache_);
    device_ptr_->destroyPipelineLayout(pipeline_layout_);
    device_ptr_->destroyDescriptorPool(descriptor_pool_);
  }

  [[nodiscard]] vk::DescriptorSet allocate_descriptor_set(
      vk::DescriptorSetLayout) const;

 protected:
  void create_descriptor_pool();

 private:
  vk::PipelineCache pipeline_cache_;
  vk::PipelineLayout pipeline_layout_;

  // vk::DescriptorAllocator descriptor_allocator_;
  vk::DescriptorPool descriptor_pool_;
};

}  // namespace core
