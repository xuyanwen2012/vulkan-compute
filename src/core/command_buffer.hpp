#pragma once

#include <iostream>
#include <limits>
#include <memory>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

template <class T> uint32_t to_u32(T value) {
  static_assert(std::is_arithmetic<T>::value, "T must be numeric");

  if (static_cast<uintmax_t>(value) >
      static_cast<uintmax_t>(std::numeric_limits<uint32_t>::max())) {
    throw std::runtime_error(
        "to_u32() failed, value is too big to be converted to uint32_t");
  }

  return static_cast<uint32_t>(value);
}

template <typename T> inline std::vector<std::byte> to_bytes(const T &value) {
  return std::vector<std::byte>{reinterpret_cast<const std::byte *>(&value),
                                reinterpret_cast<const std::byte *>(&value) +
                                    sizeof(T)};
}

namespace core {

class HPPCommandPool;
class HPPDescriptorSetLayout;

class HPPCommandBuffer {
public:
  HPPCommandBuffer(HPPCommandPool &command_pool, vk::CommandBufferLevel level)
      : command_pool{command_pool}, level{level} {
    // vk::CommandBufferAllocateInfo allocate_info(command_pool.get_handle(),
    //                                             level, 1);
  }

  HPPCommandBuffer(HPPCommandBuffer &&other);
  ~HPPCommandBuffer() {
    if (handle) {
    //   device_ptr->freeCommandBuffers(command_pool.get_handle(), handle);
    }
  };

  HPPCommandBuffer(const HPPCommandBuffer &) = delete;
  HPPCommandBuffer &operator=(const HPPCommandBuffer &) = delete;
  HPPCommandBuffer &operator=(HPPCommandBuffer &&) = delete;

  //   vk::Result begin(vk::CommandBufferUsageFlags flags, uint32_t
  //   subpass_index) {
  vk::Result begin() {
    // Reset state
    // pipeline_state.reset();
    // resource_binding_state.reset();
    descriptor_set_layout_binding_state.clear();
    stored_push_constants.clear();

    // constexpr VkCommandBufferBeginInfo begin_info{
    //     .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    // };

    vk::CommandBufferBeginInfo begin_info{};

    handle.begin(begin_info);
    return vk::Result::eSuccess;
  }

  //   void bind_pipeline_layout(vkb::core::HPPPipelineLayout &pipeline_layout);

  void push_constants(const std::vector<std::byte> &values);
  template <typename T> void push_constants(const T &value) {
    auto data = to_bytes(value);

    uint32_t size = to_u32(stored_push_constants.size() + data.size());

    if (size > max_push_constants_size) {
      std::cout << "Push constant limit exceeded (" << size << " / "
                << max_push_constants_size << " bytes)" << std::endl;
      throw std::runtime_error("Cannot overflow push constant limit");
    }

    stored_push_constants.insert(stored_push_constants.end(), data.begin(),
                                 data.end());
  }

  template <class T>
  void set_specialization_constant(uint32_t constant_id, const T &data);

  void set_specialization_constant(uint32_t constant_id,
                                   const std::vector<std::byte> &data);

  //   void bind_pipeline_layout(HPPPipelineLayout &pipeline_layout);
  void bind_buffer(const vk::Buffer &buffer, vk::DeviceSize offset,
                   vk::DeviceSize range, uint32_t set, uint32_t binding,
                   uint32_t array_element);

  vk::Result reset() { return vk::Result::eSuccess; }

private:
  const vk::CommandBufferLevel level = {};
  HPPCommandPool &command_pool;

  //   vkb::rendering::HPPPipelineState pipeline_state          = {};
  // vkb::HPPResourceBindingState     resource_binding_state  = {};

  std::vector<std::byte> stored_push_constants = {};
  uint32_t max_push_constants_size = {};
  // If true, it becomes the responsibility of the caller to update ANY
  // descriptor bindings that contain update after bind, as they wont be
  // implicitly updated
  bool update_after_bind = false;
  std::unordered_map<uint32_t, HPPDescriptorSetLayout const *>
      descriptor_set_layout_binding_state;

  // TODO: Handle
  std::shared_ptr<vk::Device> device_ptr;
  vk::CommandBuffer handle;

public:
  vk::CommandBuffer get_handle() const { return handle; }
};

template <class T>
inline void HPPCommandBuffer::set_specialization_constant(uint32_t constant_id,
                                                          const T &data) {
  set_specialization_constant(constant_id, to_bytes(data));
}

template <>
inline void
HPPCommandBuffer::set_specialization_constant<bool>(std::uint32_t constant_id,
                                                    const bool &data) {
  set_specialization_constant(constant_id, to_bytes(to_u32(data)));
}

} // namespace core
