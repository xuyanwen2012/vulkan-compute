#pragma once

#include <spdlog/spdlog.h>
#include <utility>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include "base_engine.hpp"
#include "vulkan_resource.hpp"

namespace core {
class Buffer;

using BufferReference = std::reference_wrapper<const Buffer>;

class Buffer final : public VulkanResource<vk::Buffer> {
public:
  // I provide some default flags for the buffer usage
  // For my purpose, I need to use the buffer as a storage buffer
  // And I want to use the unified shared memory
  explicit Buffer(
      std::shared_ptr<vk::Device> device_ptr, const vk::DeviceSize size,
      const vk::BufferUsageFlags buffer_usage =
          vk::BufferUsageFlagBits::eStorageBuffer |
          vk::BufferUsageFlagBits::eTransferDst,
      const VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_AUTO,
      const VmaAllocationCreateFlags flags =
          VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
          VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
          VMA_ALLOCATION_CREATE_MAPPED_BIT)
      : VulkanResource(std::move(device_ptr)), size_(size),
        persistent_{(flags & VMA_ALLOCATION_CREATE_MAPPED_BIT) != 0} {
    const auto buffer_create_info =
        vk::BufferCreateInfo().setSize(size).setUsage(buffer_usage);

    const VmaAllocationCreateInfo memory_info{
        .flags = flags,
        .usage = memory_usage,
    };

    VmaAllocationInfo allocation_info{};

    if (const auto result = vmaCreateBuffer(
            g_allocator,
            reinterpret_cast<const VkBufferCreateInfo *>(&buffer_create_info),
            &memory_info, reinterpret_cast<VkBuffer *>(&get_handle()),
            &allocation_, &allocation_info);
        result != VK_SUCCESS) {
      throw std::runtime_error("Cannot create HPPBuffer");
    }

    // log the allocation info
    spdlog::debug("Buffer::Buffer");
    spdlog::debug("\tsize: {}", allocation_info.size);
    spdlog::debug("\toffset: {}", allocation_info.offset);
    spdlog::debug("\tmemoryType: {}", allocation_info.memoryType);
    spdlog::debug("\tmappedData: {}", allocation_info.pMappedData);

    memory_ = static_cast<vk::DeviceMemory>(allocation_info.deviceMemory);
    if (persistent_) {
      mapped_data_ = static_cast<std::byte *>(allocation_info.pMappedData);
    }
  }

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

  void update(const std::vector<std::byte> &data,
              const size_t offset = 0) const {
    update(data.data(), data.size(), offset);
  }

  void update(const void *data, const size_t size,
              const size_t offset = 0) const {
    update(static_cast<const std::byte *>(data), size, offset);
  }

  void update(const std::byte *data, const size_t size,
              const size_t offset = 0) const {
    std::memcpy(mapped_data_ + offset, data, size);
  }

  template <class T>
  void convert_and_update(const T &object, const size_t offset = 0) {
    update(reinterpret_cast<const std::byte *>(&object), sizeof(T), offset);
  }

  // ---------------------------------------------------------------------------
  // The following functions provides infos for the descriptor set
  // ---------------------------------------------------------------------------

  // uint32_t memorySize() { return size_ * this->mDataTypeMemorySize; }

  [[nodiscard]] vk::DescriptorBufferInfo constructDescriptorBufferInfo() const {
    return vk::DescriptorBufferInfo()
        .setBuffer(get_handle())
        .setOffset(0)
        .setRange(size_);
  }

public:
  void destroy() override {
    if (get_handle() && allocation_ != VK_NULL_HANDLE) {
      vmaDestroyBuffer(g_allocator, get_handle(), allocation_);
    }
  }

private:
  VmaAllocation allocation_ = VK_NULL_HANDLE;
  vk::DeviceMemory memory_ = nullptr;
  vk::DeviceSize size_ = 0;
  std::byte *mapped_data_ = nullptr;

  bool persistent_ = true;
  bool mapped_ = true;
};
} // namespace core
