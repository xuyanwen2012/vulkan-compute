#pragma once

#include <iostream>
#include <optional>
#include <spirv_cross/spirv_cross.hpp>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

#include "../../shaders/all_shaders.hpp"

namespace core {

// struct ShaderBufferDesc {
//     std::shared_ptr<Buffer> buffer;
//     size_t                  offset;
//     size_t                  range;
// };

class ComputeShader {
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

  ComputeShader(std::shared_ptr<vk::Device> device_ptr, std::string_view _)
      : device_ptr(device_ptr) {

    // ------
    auto &shader_code = morton_spv;
    // ------

    const auto length = sizeof(shader_code) / sizeof(shader_code[0]);
    std::vector<uint32_t> spivrBinary(shader_code, shader_code + length);

    CollectSpirvMetaData(spivrBinary, vk::ShaderStageFlagBits::eCompute);

    vk::ShaderModuleCreateInfo create_info;
    create_info.setPCode(spivrBinary.data())
        .setCodeSize(spivrBinary.size() * sizeof(uint32_t));

    compute_shader = device_ptr->createShaderModule(create_info);
  }

  ~ComputeShader() {
    // if (compute_shader) {
    // TODO: Bug here
    if (device_ptr) {
      device_ptr->destroyShaderModule(compute_shader);
    }
    // }
  }

  friend std::ostream &operator<<(std::ostream &os, const BindMetaData &obj);
  friend std::ostream &operator<<(std::ostream &os,
                                  const PushConstantMetaData &obj);

protected:
  void CollectSpirvMetaData(std::vector<uint32_t> spivrBinary,
                            const vk::ShaderStageFlagBits shaderFlags);

  void GenerateVulkanDescriptorSetLayout();

  void GeneratePushConstantData();

private:
  vk::ShaderModule compute_shader;
  std::unordered_map<std::string, BindMetaData> m_reflectionDatas;
  std::vector<vk::DescriptorSetLayout> m_descriptorSetLayouts;
  std::vector<vk::DescriptorSet> m_descriptorSets;

  std::optional<PushConstantMetaData> m_pushConstantMeta{std::nullopt};
  std::optional<vk::PushConstantRange> m_pushConstantRange{std::nullopt};

  // Handle
  std::shared_ptr<vk::Device> device_ptr;

public:
  vk::ShaderModule &get_handle() { return compute_shader; };
};

inline std::ostream &operator<<(std::ostream &os,
                                const ComputeShader::BindMetaData &meta) {
  os << "set: " << meta.set << '\n';
  os << "binding: " << meta.binding << '\n';
  os << "count: " << meta.count << '\n';
  // os << "descriptor type: " <<  meta.descriptor_type << '\n';
  // os << "shader stage flag: " << meta.shader_stage_flag << '\n';
  return os;
}

inline std::ostream &
operator<<(std::ostream &os, const ComputeShader::PushConstantMetaData &meta) {
  os << "size: " << meta.size << '\n';
  os << "offset: " << meta.offset << '\n';
  // os << "shader stage flag: " << meta.shader_stage_flag << '\n';
  return os;
}

} // namespace core