#include "core/compute_pipeline.hpp"

vk::DescriptorSet core::ComputePipeline::allocate_descriptor_set(
    const vk::DescriptorSetLayout descriptor_set_layout) const {
  const auto set_alloc_info = vk::DescriptorSetAllocateInfo()
                                  .setDescriptorPool(descriptor_pool_)
                                  .setDescriptorSetCount(1)
                                  .setSetLayouts(descriptor_set_layout);

  return device_ptr_->allocateDescriptorSets(set_alloc_info).front();
}

void core::ComputePipeline::create_descriptor_pool() {
  // Pool size
  const std::vector pool_sizes{
      vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, 2},
  };

  // Descriptor pool
  const auto descriptor_pool_create_info =
      vk::DescriptorPoolCreateInfo().setMaxSets(1).setPoolSizes(pool_sizes);

  descriptor_pool_ =
      device_ptr_->createDescriptorPool(descriptor_pool_create_info);
}
