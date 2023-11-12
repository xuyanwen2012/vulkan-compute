#include "sequence.hpp"

[[nodiscard]] constexpr uint32_t num_blocks(const uint32_t items,
                                            const uint32_t threads_per_block) {
  return (items + threads_per_block - 1u) / threads_per_block;
}

namespace core
{
	void Sequence::record(const Algorithm& algo) const
	{
		begin();
		algo.record_bind_core(handle_);
		algo.record_bind_push(handle_);
		algo.record_dispatch(handle_);
		end();
	}

	void Sequence::begin() const
	{
		spdlog::debug("Sequence::begin!");
		constexpr auto info = vk::CommandBufferBeginInfo().setFlags(
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		handle_.begin(info);
	}

	void Sequence::end() const
	{
		spdlog::debug("Sequence::end!");
		handle_.end();
	}

	void Sequence::launch_kernel_async()
	{
		spdlog::debug("Sequence::launch_kernel_async");
		const auto submit_info = vk::SubmitInfo().setCommandBuffers(handle_);
		assert(vkh_queue_ != nullptr);
		vkh_queue_->submit(submit_info, fence_);
	}

	void Sequence::sync() const
	{
		spdlog::debug("Sequence::sync");
		const auto wait_result =
			device_ptr_->waitForFences(fence_, false, UINT64_MAX);
		assert(wait_result == vk::Result::eSuccess);
		device_ptr_->resetFences(fence_);
	}

	void Sequence::destroy()
	{
		device_ptr_->freeCommandBuffers(command_pool_, handle_);
		device_ptr_->destroyCommandPool(command_pool_);
		device_ptr_->destroyFence(fence_);
	}

	void Sequence::create_sync_objects()
	{
		fence_ = device_ptr_->createFence(vk::FenceCreateInfo());
	}

	void Sequence::create_command_pool()
	{
		spdlog::debug("Sequence::create_command_pool");

		const auto create_info =
			vk::CommandPoolCreateInfo()
			.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
			.setQueueFamilyIndex(
				vkb_device_.get_queue_index(vkb::QueueType::compute).value());

		command_pool_ = device_ptr_->createCommandPool(create_info);
	}

	void Sequence::create_command_buffer()
	{
		spdlog::debug("Sequence::create_command_buffer");

		const auto alloc_info = vk::CommandBufferAllocateInfo()
		                        .setCommandBufferCount(1)
		                        .setLevel(vk::CommandBufferLevel::ePrimary)
		                        .setCommandPool(command_pool_);

		handle_ = device_ptr_->allocateCommandBuffers(alloc_info).front();
	}
}
