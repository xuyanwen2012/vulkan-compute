#include "compute_shader.hpp"
// #include <map>

namespace core {
void ComputeShader::CollectSpirvMetaData(
    std::vector<uint32_t> spivr_binary,
    const vk::ShaderStageFlagBits shader_flags) {
  const spirv_cross::Compiler compiler(std::move(spivr_binary));
  const spirv_cross::ShaderResources resources =
      compiler.get_shader_resources();

  auto collect_resource = [&](auto resource,
                              const vk::DescriptorType descriptor_type) {
    if (!reflection_datas_.contains(resource.name)) {
      [[maybe_unused]] std::string_view resource_name = resource.name;
      const uint32_t set =
          compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
      const uint32_t binding =
          compiler.get_decoration(resource.id, spv::DecorationBinding);
      const spirv_cross::SPIRType &type = compiler.get_type(resource.type_id);
      const uint32_t type_array_size = type.array.size();
      const uint32_t count = type_array_size == 0 ? 1 : type.array[0];

      BindMetaData meta_data{set, binding, count, descriptor_type,
                             shader_flags};

      reflection_datas_[resource.name] = meta_data;
    } else {
      reflection_datas_[resource.name].shader_stage_flag |= shader_flags;
    }
  };

  for (const auto &res : resources.storage_buffers) {
    collect_resource(res, vk::DescriptorType::eStorageBuffer);
  }

  for (const auto &res : resources.push_constant_buffers) {
    [[maybe_unused]] std::string_view resource_name = res.name;
    const spirv_cross::SPIRType &type = compiler.get_type(res.type_id);
    const auto size =
        static_cast<uint32_t>(compiler.get_declared_struct_size(type));

    if (!push_constant_meta_.has_value()) {
      PushConstantMetaData meta{size, 0, shader_flags};

      push_constant_meta_ = std::optional(meta);
    } else {
      push_constant_meta_->shader_stage_flag |= shader_flags;
    }
  }
}

void ComputeShader::GenerateVulkanDescriptorSetLayout(){
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
