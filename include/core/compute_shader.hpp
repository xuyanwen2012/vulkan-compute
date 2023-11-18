#pragma once

#include <iostream>
#include <map>
#include <optional>
#include <spirv_cross/spirv_glsl.hpp>

#include "buffer.hpp"
#include "shader_loader.hpp"

namespace core {

struct ShaderBufferDesc {
  std::shared_ptr<core::Buffer> buffer;
  size_t offset;
  size_t range;
};

class ComputeShader final : public core::VulkanResource<vk::ShaderModule> {
 public:
  explicit ComputeShader(std::shared_ptr<vk::Device> device_ptr,
                         const std::string_view spirv_filename)
      : VulkanResource(device_ptr), m_spirvFilename(spirv_filename) {
    auto spirv_binary = load_shader_from_file(spirv_filename);

    const auto create_info = vk::ShaderModuleCreateInfo().setCode(spirv_binary);
    handle_ = device_ptr_->createShaderModule(create_info);

    CollectSpirvMetaData(spirv_binary);

    GenerateVulkanDescriptorSetLayout();
    GeneratePushConstantData();
  }

  ~ComputeShader() override {
    spdlog::debug("Shader::~Shader");
    destroy();
  }

  void destroy() override { device_ptr_->destroyShaderModule(handle_); }

  ComputeShader(const ComputeShader&) = delete;
  ComputeShader(ComputeShader&&) = delete;

  struct BindMetaData {
    uint32_t set;
    uint32_t binding;
    uint32_t count;
    vk::DescriptorType descriptorType;
    vk::ShaderStageFlags shaderStageFlag;
  };

  struct PushConstantMetaData {
    uint32_t size;
    uint32_t offset;
    vk::ShaderStageFlags shaderStageFlag;
  };

  void CollectSpirvMetaData(
      const std::vector<uint32_t>& spivrBinary,
      vk::ShaderStageFlags shaderFlags = vk::ShaderStageFlagBits::eCompute) {
    spirv_cross::CompilerGLSL compiler(std::move(spivrBinary));
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();

    auto collectResource = [&](auto resource,
                               vk::DescriptorType descriptorType) {
      if (m_reflectionDatas.find(resource.name) == m_reflectionDatas.end()) {
        std::string_view resourceName = resource.name;
        uint32_t set =
            compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
        uint32_t binding =
            compiler.get_decoration(resource.id, spv::DecorationBinding);
        const spirv_cross::SPIRType& type = compiler.get_type(resource.type_id);
        uint32_t typeArraySize = type.array.size();
        uint32_t count = typeArraySize == 0 ? 1 : type.array[0];
        BindMetaData metaData{set, binding, count, descriptorType, shaderFlags};
        m_reflectionDatas[resource.name] = metaData;
      } else {
        m_reflectionDatas[resource.name].shaderStageFlag |= shaderFlags;
      }
    };

    for (auto& resource : resources.storage_buffers) {
      collectResource(resource, vk::DescriptorType::eStorageBuffer);
    }

    for (const auto& resource : resources.push_constant_buffers) {
      std::string_view resourceName = resource.name;
      const spirv_cross::SPIRType& type = compiler.get_type(resource.type_id);
      uint32_t size = compiler.get_declared_struct_size(type);
      if (!m_pushConstantMeta.has_value()) {
        PushConstantMetaData meta{size, 0, shaderFlags};
        m_pushConstantMeta = std::optional<PushConstantMetaData>(meta);
      } else {
        m_pushConstantMeta->shaderStageFlag |= shaderFlags;
      }
    }

    // std:: cout << resource
    for (auto& resource : resources.storage_buffers) {
      std::cout << m_reflectionDatas[resource.name] << std::endl;
    }

    // print m_pushConstantMeta
    if (m_pushConstantMeta.has_value()) {
      std::cout << m_pushConstantMeta.value() << std::endl;
    }
  }

  void GenerateVulkanDescriptorSetLayout() {
    std::vector<vk::DescriptorSetLayoutCreateInfo>
        descriptorSetLayoutCreateInfos;
    std::vector<vk::DescriptorSetLayoutBinding> layoutBindings;
    std::map<uint32_t, std::vector<vk::DescriptorSetLayoutBinding>> m_setGroups;

    for (const auto& [resourceName, metaData] : m_reflectionDatas) {
      vk::DescriptorSetLayoutBinding binding;
      vk::ShaderStageFlags stageFlags =
          metaData.shaderStageFlag;  // almost always eCompute;
      uint32_t set = metaData.set;

      binding.setBinding(metaData.binding)
          .setDescriptorCount(metaData.count)
          .setDescriptorType(metaData.descriptorType)
          .setStageFlags(stageFlags);

      layoutBindings.push_back(binding);
      m_setGroups[set].push_back(binding);
    }

    for (const auto& [setIndex, bindingVecs] : m_setGroups) {
      vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
      descriptorSetLayoutCreateInfo.setBindingCount(bindingVecs.size())
          .setBindings(bindingVecs);

      vk::DescriptorSetLayout setLayout =
          device_ptr_->createDescriptorSetLayout(descriptorSetLayoutCreateInfo);

      m_descriptorSetLayouts.push_back(setLayout);
      // m_descriptorSets.push_back(allocater->Allocate(setLayout));
    }
  }

  void GeneratePushConstantData() {}

  friend std::ostream& operator<<(std::ostream& os,
                                  const ComputeShader::BindMetaData& metaData);
  friend std::ostream& operator<<(
      std::ostream& os, const ComputeShader::PushConstantMetaData& metaData);

 private:
  std::string m_spirvFilename;

  std::unordered_map<std::string, BindMetaData> m_reflectionDatas;

  std::unordered_map<std::string, ShaderBufferDesc> m_bufferShaderResource;

  std::vector<vk::DescriptorSetLayout> m_descriptorSetLayouts;
  std::vector<vk::DescriptorSet> m_descriptorSets;

  std::optional<PushConstantMetaData> m_pushConstantMeta{std::nullopt};
  std::optional<vk::PushConstantRange> m_pushConstantRange{std::nullopt};
};

inline std::ostream& operator<<(std::ostream& os,
                                const ComputeShader::BindMetaData& metaData) {
  os << "set: " << metaData.set << ", binding: " << metaData.binding
     << ", count: " << metaData.count
     << ", descriptorType: " << vk::to_string(metaData.descriptorType)
     << ", shaderStageFlag: " << vk::to_string(metaData.shaderStageFlag);
  return os;
}

inline std::ostream& operator<<(
    std::ostream& os, const ComputeShader::PushConstantMetaData& metaData) {
  os << "size: " << metaData.size << ", offset: " << metaData.offset
     << ", shaderStageFlag: " << vk::to_string(metaData.shaderStageFlag);
  return os;
}

};  // namespace core