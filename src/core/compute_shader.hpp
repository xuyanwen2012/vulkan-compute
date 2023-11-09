#pragma once

#include <iostream>
#include <optional>
#include <spirv_cross/spirv_cross.hpp>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

#include "../../shaders/all_shaders.hpp"
#include "vulkan_resource.hpp"

namespace core {

/**
 * @brief Shader module wrapper. For Compute Shader only.
 *
 */
class ComputeShader final : public VulkanResource<vk::ShaderModule> {
public:
  struct BindMetaData {
    uint32_t set;
    uint32_t binding;
    uint32_t count;
    vk::DescriptorType descriptor_type;
    vk::ShaderStageFlags shader_stage_flag;
  };

  struct PushConstantMetaData {
    uint32_t size;
    uint32_t offset;
    vk::ShaderStageFlags shader_stage_flag;
  };

  ComputeShader(const std::shared_ptr<vk::Device> &device_ptr,
                const std::string_view _)
      : VulkanResource(device_ptr) {
    // ------
    auto &shader_code = morton_spv;
    // ------

    const std::vector spivr_binary(shader_code,
                                   shader_code + std::size(shader_code));

    CollectSpirvMetaData(spivr_binary, vk::ShaderStageFlagBits::eCompute);

    const auto create_info = vk::ShaderModuleCreateInfo().setCode(spivr_binary);

    handle_ = device_ptr->createShaderModule(create_info);
  }

  ~ComputeShader() override { device_ptr_->destroyShaderModule(handle_); }

  friend std::ostream &operator<<(std::ostream &os, const BindMetaData &meta);
  friend std::ostream &operator<<(std::ostream &os,
                                  const PushConstantMetaData &meta);

protected:
  void CollectSpirvMetaData(std::vector<uint32_t> spivr_binary,
                            vk::ShaderStageFlagBits shader_flags);

  void GenerateVulkanDescriptorSetLayout();

  // void GeneratePushConstantData();

private:
  std::unordered_map<std::string, BindMetaData> reflection_datas_;
  std::vector<vk::DescriptorSetLayout> descriptor_set_layouts_;
  std::vector<vk::DescriptorSet> descriptor_sets_;

  std::optional<PushConstantMetaData> push_constant_meta_{std::nullopt};
  std::optional<vk::PushConstantRange> push_constant_range_{std::nullopt};
};

inline std::ostream &operator<<(std::ostream &os,
                                const ComputeShader::BindMetaData &meta) {
  os << "set: " << meta.set << '\n';
  os << "binding: " << meta.binding << '\n';
  os << "count: " << meta.count << '\n';
  os << "descriptor type: " << to_string(meta.descriptor_type) << '\n';
  os << "shader stage flag: " << to_string(meta.shader_stage_flag) << '\n';
  return os;
}

inline std::ostream &
operator<<(std::ostream &os, const ComputeShader::PushConstantMetaData &meta) {
  os << "size: " << meta.size << '\n';
  os << "offset: " << meta.offset << '\n';
  os << "shader stage flag: " << to_string(meta.shader_stage_flag) << '\n';
  return os;
}
} // namespace core
