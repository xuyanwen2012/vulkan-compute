#pragma once

#include <spdlog/spdlog.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include "vulkan_resource.hpp"

namespace core {

class Buffer;

using BufferReference = std::reference_wrapper<const Buffer>;

/**
 * @brief Buffer class is an abstraction of data, in the sense of a block of
 * memory that will be processes by the GPU. It is a warpper of Vulkan buffer
 * managed by Vulkan Memory Allocator (VMA).
 *
 * I provide some default flags for the buffer usage and memory usage. I wanted
 * to use the unified shared memory (USM), for intergrated GPUs. The data is
 * shared between CPU and GPU.
 */
class Buffer final : public VulkanResource<vk::Buffer> {
public:
  explicit Buffer(
      std::shared_ptr<vk::Device> device_ptr, vk::DeviceSize size,
      vk::BufferUsageFlags buffer_usage =
          vk::BufferUsageFlagBits::eStorageBuffer |
          vk::BufferUsageFlagBits::eTransferDst,
      VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_AUTO,
      VmaAllocationCreateFlags flags =
          VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
          VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
          VMA_ALLOCATION_CREATE_MAPPED_BIT);

  Buffer(const Buffer &) = delete;

  ~Buffer() override {
    spdlog::debug("Buffer::~Buffer");
    destroy();
  }

  Buffer &operator=(const Buffer &) = delete;
  Buffer &operator=(Buffer &&) = delete;

  [[nodiscard]] VmaAllocation get_allocation() const { return allocation_; }
  [[nodiscard]] const std::byte *get_data() const { return mapped_data_; }
  [[nodiscard]] vk::DeviceMemory get_memory() const { return memory_; }
  [[nodiscard]] vk::DeviceAddress get_device_address() const {
    return device_ptr_->getBufferAddressKHR(get_handle());
  }
  [[nodiscard]] vk::DeviceSize get_size() const { return size_; }

  void tmp_write_data(const void *data, const size_t size,
                      const size_t offset = 0) const {
    spdlog::info("Writing {} bytes to buffer", size);
    std::memcpy(mapped_data_ + offset, data, size);
  }

  void tmp_fill_zero(const size_t size, const size_t offset = 0) const {
    spdlog::info("Filling zeros {} bytes to buffer", size);
    std::memset(mapped_data_ + offset, 0, size);
  }

  // ---------------------------------------------------------------------------
  // The following functions provides infos for the descriptor set
  // ---------------------------------------------------------------------------

  [[nodiscard]] vk::DescriptorBufferInfo
  construct_descriptor_buffer_info() const;

  void destroy() override;

private:
  VmaAllocation allocation_ = VK_NULL_HANDLE;
  vk::DeviceMemory memory_ = nullptr;
  vk::DeviceSize size_ = 0;
  std::byte *mapped_data_ = nullptr;

  bool persistent_ = true;
};
} // namespace core
