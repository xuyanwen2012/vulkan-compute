#pragma once

#include "buffer.hpp"

namespace core {
// struct BufferInfo {
//   BufferReference resource;
//   uint32_t offset = 0;
// };

class CommandBuffer {
public:
  explicit CommandBuffer(const vk::CommandBuffer command_buffer)
      : handle_(command_buffer) {}

  // After begin, I should:
  // 1. Bind pipeline
  // 2. Bind descriptor set
  // 3. Push constants
  // 4. Dispatch
  // 5. End
  // 6. Submit (when I want)
  void Begin() const {
    constexpr auto begin_info = vk::CommandBufferBeginInfo().setFlags(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    handle_.begin(begin_info);
  }

  void Dispatch(uint32_t x, uint32_t y, uint32_t z);

  // Bindings
  void BindDescriptorSet(vk::PipelineBindPoint bind_point,
                         vk::PipelineLayout layout,
                         std::vector<vk::DescriptorSet> &descriptor_sets);

  // void BindPipeline(PassNode *passNode);

  //   // Commands (push const, special constants)
  //   void PushConstants(const PassNode *passNode, const uint8_t *data,
  //                      size_t size);

  //   template <typename T>
  //   void PushConstant(const PassNode *node, std::span<T> constants) {
  //     PushConstants(node, (const uint8_t *)constants.data(),
  //                   constants.size() * sizeof(T));
  //   }

  //   template <typename T>
  //   void PushConstant(const PassNode *node, const T *constants) {
  //     PushConstants(node, (const uint8_t *)constants, sizeof(T));
  //   }

private:
  vk::CommandBuffer handle_;
};
} // namespace core
