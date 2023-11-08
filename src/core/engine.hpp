#pragma once

#include <glm/fwd.hpp>
#include <memory>
#include <vulkan/vulkan.hpp>

#include "base_engine.hpp"
#include "buffer.hpp"

#include "command_buffer.hpp"
#include "command_pool.hpp"

using InputT = glm::vec4;
using OutputT = glm::uint;

[[nodiscard]] constexpr uint32_t InputSize() { return 1024; }
[[nodiscard]] constexpr uint32_t ComputeShaderProcessUnit() { return 256; }
[[nodiscard]] constexpr uint32_t div_up(const uint32_t x, const uint32_t y) {
  return (x + y - 1u) / y;
}
[[nodiscard]] constexpr uint32_t NumWorkGroup(const uint32_t items) {
  return div_up(items, ComputeShaderProcessUnit());
}

namespace core {

class ComputeEngine : BaseEngine {
public:
  ComputeEngine()
      : BaseEngine(), command_pool{std::make_shared<vk::Device>(device.device)},
        command_buffer(command_pool, vk::CommandBufferLevel::ePrimary) {}

  void run(const std::vector<InputT> &input_data) {}

protected:
private:
  // vk::CommandPool command_pool;
  // vk::CommandBuffer command_buffer;
  HPPCommandBuffer command_buffer;
  HPPCommandPool command_pool;

  vk::Pipeline pipeline;
  vk::PipelineLayout pipeline_layout;
  vk::DescriptorSetLayout descriptor_set_layout;
  vk::DescriptorPool descriptor_pool;
  vk::DescriptorSet descriptor_set;

  std::vector<Buffer> usm_buffers;
};

} // namespace core