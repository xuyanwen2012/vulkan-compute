#pragma once

#include <memory>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include "base_engine.hpp"

namespace core {

class Buffer {
public:
  // I provide some default flags for the buffer usage
  // For my purpose, I need to use the buffer as a storage buffer
  // And I want to use the unified shared memory
  Buffer(const vk::DeviceSize size_,
         const vk::BufferUsageFlags buffer_usage =
             vk::BufferUsageFlagBits::eStorageBuffer |
             vk::BufferUsageFlagBits::eTransferDst,
         const VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_AUTO,
         const VmaAllocationCreateFlags flags =
             VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
             VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
             VMA_ALLOCATION_CREATE_MAPPED_BIT)
      : size(size_),
        persistent{(flags & VMA_ALLOCATION_CREATE_MAPPED_BIT) != 0} {

    const vk::BufferCreateInfo buffer_create_info({}, size, buffer_usage);

    const VmaAllocationCreateInfo memory_info{
        .flags = flags,
        .usage = memory_usage,
    };

    VmaAllocationInfo allocation_info{};
    auto result = vmaCreateBuffer(
        g_allocator,
        reinterpret_cast<const VkBufferCreateInfo *>(&buffer_create_info),
        &memory_info, reinterpret_cast<VkBuffer *>(&get_handle()), &allocation,
        &allocation_info);

    if (result != VK_SUCCESS) {
      throw std::runtime_error("Cannot create HPPBuffer");
    }

    if constexpr (true) {
      std::cout << "alloc_info: " << std::endl;
      std::cout << "\tsize: " << allocation_info.size << std::endl;
      std::cout << "\toffset: " << allocation_info.offset << std::endl;
      std::cout << "\tmemoryType: " << allocation_info.memoryType << std::endl;
      std::cout << "\tmappedData: " << allocation_info.pMappedData << std::endl;
      std::cout << "\tdeviceMemory: " << allocation_info.deviceMemory
                << std::endl;
    }

    memory = static_cast<vk::DeviceMemory>(allocation_info.deviceMemory);
    if (persistent) {
      mapped_data = static_cast<std::byte *>(allocation_info.pMappedData);
    }
  }

  Buffer(const Buffer &) = delete;

  ~Buffer() {
    if (get_handle() && (allocation != VK_NULL_HANDLE)) {
      //   unmap();
      vmaDestroyBuffer(g_allocator, static_cast<VkBuffer>(get_handle()),
                       allocation);
    }
  }

  Buffer &operator=(const Buffer &) = delete;
  Buffer &operator=(Buffer &&) = delete;

  VmaAllocation get_allocation() const { return allocation; };
  const std::byte *get_data() const { return mapped_data; };
  vk::DeviceMemory get_memory() const { return memory; };
  uint64_t get_device_address() const {
    return device_ptr->getBufferAddressKHR(get_handle());
  };
  vk::DeviceSize get_size() const { return size; };

  void update(const std::vector<std::byte> &data, size_t offset) {
    update(data.data(), data.size(), offset);
  }

  void update(const void *data, const size_t size, const size_t offset) {
    update(reinterpret_cast<const std::byte *>(data), size, offset);
  }

  void update(const std::byte *data, const size_t size, const size_t offset) {
    std::memcpy(mapped_data + offset, data, size);
  }

private:
  VmaAllocation allocation = VK_NULL_HANDLE;
  vk::DeviceMemory memory = nullptr;
  const vk::DeviceSize size = 0;
  std::byte *mapped_data = nullptr;

  const bool persistent = false;
  bool mapped = false;

  // handle

  // TODO: Fix this
  inline vk::Buffer &get_handle() { return buffer; };
  inline const vk::Buffer &get_handle() const { return buffer; }
  vk::Buffer buffer;
  std::shared_ptr<vk::Device> device_ptr;
};

} // namespace core