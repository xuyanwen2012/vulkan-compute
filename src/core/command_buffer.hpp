// #pragma once

// #include "buffer.hpp"

// namespace core {

// struct BufferInfo {
//   BufferReference resource;
//   uint32_t offset = 0;
// };

// class CommandBuffer {
// public:
//   CommandBuffer(vk::CommandBuffer commandBuffer)
//       : m_handle(std::move(commandBuffer)) {}

//   // After begin, I should:
//   // 1. Bind pipeline
//   // 2. Bind descriptor set
//   // 3. Push constants
//   // 4. Dispatch
//   // 5. End
//   // 6. Submit (when I want)
//   void Begin() {
//     vk::CommandBufferBeginInfo commandBufferBeginInfo;
//     commandBufferBeginInfo.setFlags(
//         vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
//     m_handle.begin(commandBufferBeginInfo);
//   }

//   void Dispatch(uint32_t x, uint32_t y, uint32_t z);

//   // Bindings
//   void BindDescriptorSet(vk::PipelineBindPoint bindPoint,
//                          vk::PipelineLayout layout,
//                          std::vector<vk::DescriptorSet> &descriptorSets);

//   //   void BindPipeline(PassNode *passNode);

//   //   // Commands (push const, special constants)
//   //   void PushConstants(const PassNode *passNode, const uint8_t *data,
//   //                      size_t size);

//   //   template <typename T>
//   //   void PushConstant(const PassNode *node, std::span<T> constants) {
//   //     PushConstants(node, (const uint8_t *)constants.data(),
//   //                   constants.size() * sizeof(T));
//   //   }

//   //   template <typename T>
//   //   void PushConstant(const PassNode *node, const T *constants) {
//   //     PushConstants(node, (const uint8_t *)constants, sizeof(T));
//   //   }

// private:
//   vk::CommandBuffer m_handle;
// };

// } // namespace core