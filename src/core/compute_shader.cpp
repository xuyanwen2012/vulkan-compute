#include "core/compute_shader.hpp"

#include <iostream>
#include <map>
#include <ranges>
#include <spirv_cross/spirv_glsl.hpp>

#include "core/compute_pipeline.hpp"
#include "core/shader_loader.hpp"

namespace core {
ComputeShader::ComputeShader(std::shared_ptr<vk::Device> device_ptr,
                             const ComputePipeline* compute_pipeline,
                             const std::string_view spirv_filename)
    : VulkanResource(std::move(device_ptr)),
      m_spirv_filename_(spirv_filename),
      compute_pipeline_(compute_pipeline) {
  const auto spirv_binary = load_shader_from_file(spirv_filename);

  const auto create_info = vk::ShaderModuleCreateInfo().setCode(spirv_binary);
  handle_ = device_ptr_->createShaderModule(create_info);

  collect_spirv_meta_data(spirv_binary);

  generate_vulkan_descriptor_set_layout();
  generate_push_constant_data();
}

void ComputeShader::bind(const std::string& resource_name,
                         const ShaderBufferDesc& buffer_desc) {
  if (!m_reflection_data_.contains(resource_name)) {
    throw std::runtime_error("Fail to find shader resource");
  }

  const vk::DescriptorBufferInfo buffer_info{
      buffer_desc.buffer->get_handle(),
      buffer_desc.offset,
      buffer_desc.range,  // size of buffer
  };

  const auto bind_data = m_reflection_data_[resource_name];
  const auto writer =
      vk::WriteDescriptorSet()
          .setDescriptorType(bind_data.descriptor_type)  //  eStorageBuffer
          .setDescriptorCount(bind_data.count)           // 1
          .setDstBinding(bind_data.binding)
          .setBufferInfo(buffer_info)
          .setDstSet(m_descriptor_sets_[bind_data.set]);

  device_ptr_->updateDescriptorSets(1, &writer, 0, nullptr);
}

void ComputeShader::collect_spirv_meta_data(
    const std::vector<uint32_t>& spirv_binary,
    vk::ShaderStageFlags shader_flags) {
  spdlog::info("ComputeShader::collect_spirv_meta_data {}",
               spirv_binary.size());

  const spirv_cross::CompilerGLSL compiler(spirv_binary);
  spirv_cross::ShaderResources resources = compiler.get_shader_resources();

  auto collect_resource = [&](auto resource,
                              const vk::DescriptorType descriptor_type) {
    if (!m_reflection_data_.contains(resource.name)) {
      // [[maybe_unused]] const std::string_view resource_name = resource.name;

      const uint32_t set =
          compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
      const uint32_t binding =
          compiler.get_decoration(resource.id, spv::DecorationBinding);

      const spirv_cross::SPIRType& type = compiler.get_type(resource.type_id);

      const uint32_t count = type.array.empty() ? 1 : type.array[0];

      BindMetaData meta_data{
          set, binding, count, descriptor_type, shader_flags};

      m_reflection_data_[resource.name] = meta_data;
    } else {
      m_reflection_data_[resource.name].shader_stage_flag |= shader_flags;
    }
  };

  for (const auto& resource : resources.storage_buffers) {
    collect_resource(resource, vk::DescriptorType::eStorageBuffer);
  }

  for (const auto& resource : resources.push_constant_buffers) {
    if (!m_push_constant_meta_.has_value()) {
      const spirv_cross::SPIRType& type = compiler.get_type(resource.type_id);
      const uint32_t size = compiler.get_declared_struct_size(type);

      PushConstantMetaData meta{size, 0, shader_flags};
      m_push_constant_meta_ = std::optional(meta);
    } else {
      m_push_constant_meta_->shader_stage_flag |= shader_flags;
    }
  }

  for (const auto& resource : resources.storage_buffers) {
    std::cout << m_reflection_data_[resource.name] << std::endl;
  }

  if (m_push_constant_meta_.has_value()) {
    std::cout << m_push_constant_meta_.value() << std::endl;
  }
}

void ComputeShader::generate_vulkan_descriptor_set_layout() {
  // Generate based on which set? what is the binding number
  std::vector<vk::DescriptorSetLayoutBinding> layout_bindings;
  std::map<uint32_t, std::vector<vk::DescriptorSetLayoutBinding>> m_set_groups;

  for (const auto& meta_data : m_reflection_data_ | std::views::values) {
    const auto set_layout_binding =
        vk::DescriptorSetLayoutBinding()
            .setBinding(meta_data.binding)
            .setDescriptorCount(meta_data.count)
            .setDescriptorType(meta_data.descriptor_type)
            .setStageFlags(meta_data.shader_stage_flag);

    layout_bindings.push_back(set_layout_binding);
    m_set_groups[meta_data.set].push_back(set_layout_binding);
  }

  for (const auto& binding_vecs : m_set_groups | std::views::values) {
    const auto descriptor_set_layout_create_info =
        vk::DescriptorSetLayoutCreateInfo()
            .setBindingCount(binding_vecs.size())
            .setBindings(binding_vecs);

    vk::DescriptorSetLayout set_layout = device_ptr_->createDescriptorSetLayout(
        descriptor_set_layout_create_info);

    m_descriptor_set_layouts_.push_back(set_layout);

    m_descriptor_sets_.push_back(
        compute_pipeline_->allocate_descriptor_set(set_layout));
  }
}

void ComputeShader::generate_push_constant_data() {
  if (m_push_constant_meta_.has_value()) {
    const auto range =
        vk::PushConstantRange()
            .setOffset(m_push_constant_meta_->offset)
            .setSize(m_push_constant_meta_->size)
            .setStageFlags(m_push_constant_meta_->shader_stage_flag);

    m_push_constant_range_ = std::optional{range};
  }
}

std::ostream& operator<<(std::ostream& os,
                         const ComputeShader::BindMetaData& meta_data) {
  os << "set: " << meta_data.set << ", binding: " << meta_data.binding
     << ", count: " << meta_data.count
     << ", descriptorType: " << to_string(meta_data.descriptor_type)
     << ", shaderStageFlag: " << to_string(meta_data.shader_stage_flag);
  return os;
}

std::ostream& operator<<(std::ostream& os,
                         const ComputeShader::PushConstantMetaData& meta_data) {
  os << "size: " << meta_data.size << ", offset: " << meta_data.offset
     << ", shaderStageFlag: " << to_string(meta_data.shader_stage_flag);
  return os;
}
}  // namespace core
