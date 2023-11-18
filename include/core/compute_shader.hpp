#pragma once

#include <optional>

#include "buffer.hpp"

namespace core {

class ComputePipeline;

struct ShaderBufferDesc {
  std::shared_ptr<Buffer> buffer;
  size_t offset;
  size_t range;
};

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

  explicit ComputeShader(std::shared_ptr<vk::Device> device_ptr,
                         const ComputePipeline* compute_pipeline,
                         std::string_view spirv_filename);

  ~ComputeShader() override {
    spdlog::debug("Shader::~Shader");
    destroy();
  }

  void destroy() override { device_ptr_->destroyShaderModule(handle_); }

  ComputeShader(const ComputeShader&) = delete;
  ComputeShader(ComputeShader&&) = delete;

  [[nodiscard]] auto get_shader_reflection_data() const {
    return m_reflection_data_;
  }
  [[nodiscard]] auto& get_descriptor_set_layouts() const {
    return m_descriptor_set_layouts_;
  }
  [[nodiscard]] auto& get_descriptor_set() { return m_descriptor_sets_; }
  [[nodiscard]] auto& get_push_constant_range() {
    return m_push_constant_range_;
  }

  void bind(const std::string& resource_name,
            const ShaderBufferDesc& buffer_desc);

 protected:
  void collect_spirv_meta_data(
      const std::vector<uint32_t>& spirv_binary,
      vk::ShaderStageFlags shader_flags = vk::ShaderStageFlagBits::eCompute);

  void generate_vulkan_descriptor_set_layout();

  void generate_push_constant_data();

  friend std::ostream& operator<<(std::ostream& os,
                                  const BindMetaData& meta_data);
  friend std::ostream& operator<<(std::ostream& os,
                                  const PushConstantMetaData& meta_data);

 private:
  std::string m_spirv_filename_;
  const ComputePipeline* compute_pipeline_;

  std::unordered_map<std::string, BindMetaData> m_reflection_data_;

  std::unordered_map<std::string, ShaderBufferDesc> m_buffer_shader_resource_;

  std::vector<vk::DescriptorSetLayout> m_descriptor_set_layouts_;
  std::vector<vk::DescriptorSet> m_descriptor_sets_;

  std::optional<PushConstantMetaData> m_push_constant_meta_{std::nullopt};
  std::optional<vk::PushConstantRange> m_push_constant_range_{std::nullopt};
};

}  // namespace core
