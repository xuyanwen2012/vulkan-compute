#include "compute_shader.hpp"

void core::ComputeShader::CollectSpirvMetaData(
    std::vector<uint32_t> spivrBinary,
    const VkShaderStageFlagBits shaderFlags) {
  spirv_cross::Compiler compiler(std::move(spivrBinary));
  spirv_cross::ShaderResources resources = compiler.get_shader_resources();

  auto collectResource = [&](auto resource, VkDescriptorType descriptorType) {
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

      std::cout << metaData << std::endl;

      m_reflectionDatas[resource.name] = metaData;
    } else {
      m_reflectionDatas[resource.name].shader_stage_flag |= shaderFlags;
    }
  };

  std::cout << "Storage Buffer: \n";
  for (const auto &res : resources.storage_buffers) {
    collectResource(res, VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
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

void core::ComputeShader::GenerateVulkanDescriptorSetLayout() {

  std::vector<VkDescriptorSetLayoutCreateInfo> descriptorSetLayoutCreateInfos;
  std::vector<VkDescriptorSetLayoutBinding> layoutBindings;

  std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> m_setGroups;

  for (const auto &[resourceName, metaData] : m_reflectionDatas) {
    VkDescriptorSetLayoutBinding binding;
    VkShaderStageFlags stageFlags = metaData.shader_stage_flag;
    uint32_t set = metaData.set;

    binding = {
        .binding = metaData.binding,
        .descriptorType = metaData.descriptor_type,
        .descriptorCount = metaData.count,
        .stageFlags = stageFlags,
    };

    layoutBindings.push_back(binding);
    m_setGroups[set].push_back(binding);
  }

  for (const auto &[setIndex, bindingVecs] : m_setGroups) {
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindingVecs.size()),
        .pBindings = bindingVecs.data(),
    };

    VkDescriptorSetLayout setLayout;

    if (const auto result = disp.createDescriptorSetLayout(
            &descriptorSetLayoutCreateInfo, nullptr, &setLayout);
        result != VK_SUCCESS) {
      throw std::runtime_error("Cannot create descriptor set layout");
    }

    m_descriptorSetLayouts.push_back(setLayout);
    //   m_descriptorSets.push_back(allocater->Allocate(setLayout));
  }
};

void GeneratePushConstantData() {}