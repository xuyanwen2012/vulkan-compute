#include "buffer.hpp"

#include "base_engine.hpp"

namespace core
{
	Buffer::Buffer(std::shared_ptr<vk::Device> device_ptr, const vk::DeviceSize size,
		const vk::BufferUsageFlags buffer_usage, const VmaMemoryUsage memory_usage,
		const VmaAllocationCreateFlags flags): VulkanResource(std::move(device_ptr)), size_(size),
		                                       persistent_{(flags & VMA_ALLOCATION_CREATE_MAPPED_BIT) != 0}
	{
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

	vk::DescriptorBufferInfo Buffer::construct_descriptor_buffer_info() const
	{
		return vk::DescriptorBufferInfo()
		       .setBuffer(get_handle())
		       .setOffset(0)
		       .setRange(size_);
	}

	void Buffer::destroy()
	{
		if (get_handle() && allocation_ != VK_NULL_HANDLE) {
			vmaDestroyBuffer(g_allocator, get_handle(), allocation_);
		}
	}
}
