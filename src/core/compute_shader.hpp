#pragma once

#include <iostream>
#include <map>
#include <optional>
#include <spirv_cross/spirv_cross.hpp>
#include <unordered_map>

#include "../../shaders/morton_shader.hpp"
#include "base.hpp"

namespace core {

// struct ShaderBufferDesc {
//     std::shared_ptr<Buffer> buffer;
//     size_t                  offset;
//     size_t                  range;
// };

class ComputeShader : public Base {
public:
  struct BindMetaData {
    uint32_t set;
    uint32_t binding;
    uint32_t count;
    VkDescriptorType descriptor_type;
    VkShaderStageFlags shader_stage_flag;
  };

  struct PushConstantMetaData {
    uint32_t size;
    uint32_t offset;
    VkShaderStageFlags shader_stage_flag;
  };

  ComputeShader(vkb::DispatchTable &disp, std::string_view _) : Base(disp) {
    auto &device = disp.device;

    auto length = sizeof(morton_spv) / sizeof(morton_spv[0]);
    std::vector<uint32_t> spivrBinary(morton_spv, morton_spv + length);

    CollectSpirvMetaData(spivrBinary,
                         VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT);

    const VkShaderModuleCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spivrBinary.size() * sizeof(uint32_t),
        .pCode = spivrBinary.data(),
    };

    if (const auto result =
            disp.createShaderModule(&create_info, nullptr, &compute_shader);
        result != VK_SUCCESS) {
      throw std::runtime_error("Cannot create shader module");
    }
  }

  ~ComputeShader() { disp.destroyShaderModule(compute_shader, nullptr); }

  friend std::ostream &operator<<(std::ostream &os, const BindMetaData &obj);
  friend std::ostream &operator<<(std::ostream &os,
                                  const PushConstantMetaData &obj);

protected:
  void CollectSpirvMetaData(std::vector<uint32_t> spivrBinary,
                            const VkShaderStageFlagBits shaderFlags);

  void GenerateVulkanDescriptorSetLayout();

  void GeneratePushConstantData();

private:
  VkShaderModule compute_shader;
  std::unordered_map<std::string, BindMetaData> m_reflectionDatas;
  std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
  std::vector<VkDescriptorSet> m_descriptorSets;

  std::optional<PushConstantMetaData> m_pushConstantMeta{std::nullopt};
  std::optional<VkPushConstantRange> m_pushConstantRange{std::nullopt};
};

inline std::ostream &operator<<(std::ostream &os,
                                const ComputeShader::BindMetaData &meta) {
  os << "set: " << meta.set << '\n';
  os << "binding: " << meta.binding << '\n';
  os << "count: " << meta.count << '\n';
  os << "descriptor type: " << meta.descriptor_type << '\n';
  os << "shader stage flag: " << meta.shader_stage_flag << '\n';
  return os;
}

inline std::ostream &
operator<<(std::ostream &os, const ComputeShader::PushConstantMetaData &meta) {
  os << "size: " << meta.size << '\n';
  os << "offset: " << meta.offset << '\n';
  os << "shader stage flag: " << meta.shader_stage_flag << '\n';
  return os;
}

} // namespace core