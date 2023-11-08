#pragma once

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace core {
class DescriptorAllocator {
public:
  struct PoolSizes {
    std::vector<std::pair<VkDescriptorType, float>> sizes = {
        // {VkDescriptorType::eSampler, 0.5f},
        // {VkDescriptorType::eCombinedImageSampler, 4.f},
        // {VkDescriptorType::eUniformBuffer, 2.f},
        {VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f},
        // {VkDescriptorType::eSampledImage, 2.f},
        // {VkDescriptorType::eInputAttachment, 0.5f},
        // {VkDescriptorType::eUniformBufferDynamic, 0.5f},
        // {VkDescriptorType::eStorageBufferDynamic, 0.5f}};
    };
  };
  VkDescriptorSet Allocate(VkDescriptorSetLayout);

  VkDevice m_device;
  VkDescriptorPool GrabPool();

  PoolSizes descriptorSizes;

  VkDescriptorPool m_currentPool{nullptr};
  std::vector<VkDescriptorPool> m_usedPools;
  std::vector<VkDescriptorPool> m_freePools;
};

class DescriptorBuilder {
public:
  static DescriptorBuilder Begin( // DescriptorLayoutCache *cache,
      DescriptorAllocator *allocator);
  static DescriptorBuilder Begin();

  DescriptorBuilder &BindBuffer(uint32_t binding,
                                VkDescriptorBufferInfo bufferInfo,
                                VkDescriptorType descriptorType,
                                VkShaderStageFlags stageFlags);
//   DescriptorBuilder &BindImage(uint32_t binding,
//                                VkDescriptorImageInfo imageInfo,
//                                VkDescriptorType descriptorType,
//                                VkShaderStageFlags stageFlags);

  bool Build(VkDescriptorSet &set, VkDescriptorSetLayout &layout);
  bool Build(VkDescriptorSet &set);

private:
  std::vector<VkWriteDescriptorSet> m_writes;
  std::vector<VkDescriptorSetLayoutBinding> m_bindings;

  //   std::shared_ptr<DescriptorLayoutCache> m_cache;
  std::shared_ptr<DescriptorAllocator> m_alloc;
};

}; // namespace core