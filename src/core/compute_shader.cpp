#include "compute_shader.hpp"
#include <map>

namespace core {

void ComputeShader::CollectSpirvMetaData(
    std::vector<uint32_t> spivrBinary,
    const vk::ShaderStageFlagBits shaderFlags) {
  spirv_cross::Compiler compiler(std::move(spivrBinary));
  spirv_cross::ShaderResources resources = compiler.get_shader_resources();

  auto collectResource = [&](auto resource, vk::DescriptorType descriptorType) {
    if (m_reflectionDatas.find(resource.name) == m_reflectionDatas.end()) {
      std::string_view resourceName = resource.name;
      uint32_t set =
          compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
      uint32_t binding =
          compiler.get_decoration(resource.id, spv::DecorationBinding);
      const spirv_cross::SPIRType &type = compiler.get_type(resource.type_id);
      uint32_t typeArraySize = type.array.size();
      uint32_t count = typeArraySize == 0 ? 1 : type.array[0];
      BindMetaData metaData{set, binding, count, descriptorType, shaderFlags};
      m_reflectionDatas[resource.name] = metaData;
    } else {
      m_reflectionDatas[resource.name].shader_stage_flag |= shaderFlags;
    }
  };

  std::cout << "Storage Buffer: \n";
  for (const auto &res : resources.storage_buffers) {
    collectResource(res, vk::DescriptorType::eStorageBuffer);
  }

  std::cout << "Push Constant: \n";
  for (const auto &res : resources.push_constant_buffers) {
    std::string_view resourceName = res.name;
    const spirv_cross::SPIRType &type = compiler.get_type(res.type_id);
    uint32_t size = compiler.get_declared_struct_size(type);
    if (!m_pushConstantMeta.has_value()) {
      PushConstantMetaData meta{size, 0, shaderFlags};

      std::cout << meta << std::endl;

      m_pushConstantMeta = std::optional<PushConstantMetaData>(meta);
    } else {
      m_pushConstantMeta->shader_stage_flag |= shaderFlags;
    }
  }
}

void core::ComputeShader::GenerateVulkanDescriptorSetLayout(){

    // std::vector<vk::DescriptorSetLayoutCreateInfo>
    // descriptorSetLayoutCreateInfos;
    // std::vector<vk::DescriptorSetLayoutBinding> layoutBindings;

    // std::map<uint32_t, std::vector<vk::DescriptorSetLayoutBinding>>
    // m_setGroups;

    // for (const auto &[resourceName, metaData] : m_reflectionDatas) {
    //   vk::DescriptorSetLayoutBinding binding;
    //   vk::ShaderStageFlags stageFlags = metaData.shader_stage_flag;
    //   uint32_t set = metaData.set;

    //   binding.setBinding(metaData.binding)
    //       .setDescriptorCount(metaData.count)
    //       .setDescriptorType(metaData.descriptor_type)
    //       .setStageFlags(stageFlags);

    //   layoutBindings.push_back(binding);
    //   m_setGroups[set].push_back(binding);
    // }

    // for (const auto &[setIndex, bindingVecs] : m_setGroups) {
    //   vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
    //   descriptorSetLayoutCreateInfo.setBindingCount(bindingVecs.size())
    //       .setBindings(bindingVecs);
    //   vk::DescriptorSetLayout setLayout =
    //       device_ptr->createDescriptorSetLayout(descriptorSetLayoutCreateInfo);

    //   m_descriptorSetLayouts.push_back(setLayout);
    //   // m_descriptorSets.push_back(g_allocator.Allocate(setLayout));
    // }
};

void GeneratePushConstantData() {}

} // namespace core
