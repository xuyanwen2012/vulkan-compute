#pragma once

#include "buffer.hpp"
#include "vulkan_resource.hpp"
#include <cstdint>

namespace core {

using WorkGroup = std::array<uint32_t, 3>;

/**
 * @brief Algorithm is an abstraction of a compute shader. It creates compute
 * pipeline for this shader, and creates the necessary components to it.
 */
class Algorithm final : public VulkanResource<vk::ShaderModule> {

public:
  explicit Algorithm(std::shared_ptr<vk::Device> device_ptr,
                     std::string_view spirv_filename,
                     const std::vector<std::shared_ptr<Buffer>> &buffers,
                     uint32_t threads_per_block,
                     const std::vector<float> &push_constants = {});

  ~Algorithm() override {
    spdlog::debug("YxAlgorithm::~YxAlgorithm");
    destroy();
  }

  void destroy() override;

  template <typename T>
  void set_push_constants(const std::vector<T> &push_constants) {
    spdlog::debug("YxAlgorithm::set_push_constants", push_constants.size());
    const uint32_t memory_size = sizeof(decltype(push_constants.back()));
    const uint32_t size = push_constants.size();
    this->set_push_constants(push_constants.data(), size, memory_size);
  }

  void set_push_constants(const void *data, uint32_t size,
                          uint32_t memory_size);

  template <typename T> std::vector<T> get_push_constants() {
    return {static_cast<T *>(push_constants_data_),
            static_cast<T *>(push_constants_data_) + push_constants_size_};
  }

  // Used by Sequence (cmd buffer)

  /**
   * @brief Let the cmd_buffer to bind my pipeline and descriptor set
   *
   * @param cmd_buf
   */
  void record_bind_core(const vk::CommandBuffer &cmd_buf) const;

  /**
   * @brief Let the cmd_buffer to bind my push constants. 
   *
   * @param cmd_buf
   */
  void record_bind_push(const vk::CommandBuffer &cmd_buf) const;

  [[deprecated("Use tmp")]] void
  record_dispatch(const vk::CommandBuffer &cmd_buf) const;

  void record_dispatch_tmp(const vk::CommandBuffer &cmd_buf,
                           uint32_t data_size) const;

protected:
  // Basically setup the buffer, its descriptor set, binding etc.
  void create_parameters();
  void create_pipeline();
  void create_shader_module();

private:
  std::string spirv_filename_;

  vk::Pipeline pipeline_;
  vk::PipelineCache pipeline_cache_;
  vk::PipelineLayout pipeline_layout_;
  vk::DescriptorSetLayout descriptor_set_layout_;
  vk::DescriptorPool descriptor_pool_;
  vk::DescriptorSet descriptor_set_;

  // In CUDA terms, this is the number threads per block
  // or work-items per work-group
  // WorkGroup workgroup_size_;

  // uint32_t
  uint32_t threads_per_block_;

  std::vector<std::shared_ptr<Buffer>> usm_buffers_;

  // Currently this assume the push constant is homogeneous type
  void *push_constants_data_ = nullptr;
  uint32_t push_constants_data_type_memory_size_ = 0;
  uint32_t push_constants_size_ = 0;
};

} // namespace core