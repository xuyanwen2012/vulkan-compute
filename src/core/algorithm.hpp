#pragma once

#include "buffer.hpp"
#include "compute_shader.hpp"
#include <array>
#include <memory>
#include <utility>
#include <vulkan/vulkan.hpp>
#include "../../shaders/all_shaders.hpp"

namespace core
{
	using Workgroup = std::array<uint32_t, 3>;

	constexpr uint32_t kPushConstStartingOffset = 16;


	// Abstraction of a GPU Computation (kernel function), which is a Shader Module
	// and a Pipeline
	class Algorithm final : public VulkanResource<vk::ShaderModule>
	{
	public:
		explicit Algorithm(std::shared_ptr<vk::Device> device,
		                   // const std::vector<uint32_t> &spirv = {},
		                   const Workgroup& workgroup = {},
		                   const std::vector<uint32_t>& specialization_constants = {})
			: VulkanResource(std::move(device))
		{
			rebuild(workgroup, specialization_constants);
		}

		~Algorithm() override { destroy(); }

		// Rebuild function to reconstruct algorithm with configuration parameters
		//  * to create the underlying resources
		void rebuild(
			// const std::vector<uint32_t> &spirv = {},
			const Workgroup& workgroup = {},
			const std::vector<uint32_t>& specialization_constants = {})
		{
			if (!specialization_constants.empty())
			{
				if (this->m_specialization_constants_data_)
				{
					free(this->m_specialization_constants_data_);
				}
				constexpr uint32_t memory_size = sizeof(decltype(specialization_constants.back()));
				const auto size = static_cast<uint32_t>(specialization_constants.size());
				const uint32_t total_size = size * memory_size;
				this->m_specialization_constants_data_ = malloc(total_size);
				memcpy(this->m_specialization_constants_data_, specialization_constants.data(),
				       total_size);
				this->m_specialization_constants_data_type_memory_size_ = memory_size;
				this->m_specialization_constants_size_ = size;
			}

			set_workgroup(workgroup);

			create_parameters();
			create_shader_module();
			create_pipeline();
		}

		template <typename T>
		void set_push_constants(const std::vector<T>& push_constants)
		{
			const uint32_t memory_size = sizeof(decltype(push_constants.back()));
			const uint32_t size = push_constants.size();
			this->set_push_constants(push_constants.data(), size, memory_size);
		}

		void set_push_constants(const void* data, const uint32_t size, const uint32_t memory_size)
		{
			const uint32_t total_size = memory_size * size;

			if (const uint32_t previous_total_size =
					this->m_push_constants_size_ * this->m_push_constants_data_type_memory_size_;
				total_size != previous_total_size)
			{
				throw std::runtime_error("totalSize != previousTotalSize");
			}
			if (this->m_push_constants_data_)
			{
				free(this->m_push_constants_data_);
			}

			this->m_push_constants_data_ = malloc(total_size);
			memcpy(this->m_push_constants_data_, data, total_size);
			this->m_push_constants_data_type_memory_size_ = memory_size;
			this->m_push_constants_size_ = size;
		}

		void record_dispatch(const vk::CommandBuffer& command_buffer) const
		{
			command_buffer.dispatch(m_workgroup_[0], m_workgroup_[1], m_workgroup_[2]);
		}

		// binding pipeline etc
		void record_bind_core(const vk::CommandBuffer& command_buffer) const
		{
			command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline_);
			command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
			                                  pipeline_layout_,
			                                  0, // First set
			                                  descriptor_set_,
			                                  nullptr // Dispatcher
			);
		}

		void record_bind_push(const vk::CommandBuffer& command_buffer) const
		{
			// immediate_command_buffer_.pushConstants(
			//     pipeline_layout_,
			//     vk::ShaderStageFlagBits::eCompute, 0,
			//     sizeof(default_push), default_push);

			// constexpr MyPushConst push_const{InputSize(), 0.0f,
			// 1024.0f}; immediate_command_buffer_.pushConstants(
			//     pipeline_layout_,
			//     vk::ShaderStageFlagBits::eCompute, 16,
			//     sizeof(push_const), &push_const);

			if (m_push_constants_size_)
			{
				//constexpr uint32_t default_push[3]{0, 0, 0};
				//command_buffer.pushConstants(
				//	pipeline_layout_,
				//	vk::ShaderStageFlagBits::eCompute, 0,
				//	sizeof(default_push), default_push);

				//constexpr std::array push_const{1024.0f, 0.0f, 1024.0f};
				//command_buffer.pushConstants(
				//	pipeline_layout_,
				//	vk::ShaderStageFlagBits::eCompute,
				//	kPushConstStartingOffset,
				//	sizeof(push_const), &push_const);

				command_buffer.pushConstants(pipeline_layout_,
				                             vk::ShaderStageFlagBits::eCompute,
				                             0,
				                             m_push_constants_size_ * m_push_constants_data_type_memory_size_,
				                             this->m_push_constants_data_);
			}
		}

		[[nodiscard]] const Workgroup& get_workgroup() const { return m_workgroup_; }

		void set_workgroup(const Workgroup& workgroup) { m_workgroup_ = workgroup; }

	protected:
		void create_shader_module()
		{
			//compute_module_ = std::make_shared<ComputeShader>(device_ptr_, "None");
			// ------
			auto& shader_code = morton_spv;
			// ------

			const std::vector spivr_binary(shader_code,
			                               shader_code + std::size(shader_code));

			const auto create_info = vk::ShaderModuleCreateInfo().setCode(spivr_binary);
			handle_ = device_ptr_->createShaderModule(create_info);
		}

		void create_pipeline()
		{
			const auto push_const =
				vk::PushConstantRange()
				.setStageFlags(vk::ShaderStageFlagBits::eCompute)
				.setOffset(0)
				.setSize(this->m_push_constants_data_type_memory_size_ *
					this->m_push_constants_size_);

			// Pipeline layout (2/3)
			const auto layout_create_info = vk::PipelineLayoutCreateInfo()
			                                .setSetLayoutCount(1)
			                                .setSetLayouts(descriptor_set_layout_)
			                                .setPushConstantRangeCount(1)
			                                .setPushConstantRanges(push_const);
			pipeline_layout_ = device_ptr_->createPipelineLayout(layout_create_info);

			// Pipeline cache (2.5/3)
			constexpr auto pipeline_cache_info = vk::PipelineCacheCreateInfo();
			pipeline_cache_ = device_ptr_->createPipelineCache(pipeline_cache_info);

			// Pipeline itself (3/3)
			// For CLSPV generated shader, need to use specialization constants to pass
			// the number of "ThreadsPerBlock" (CUDA term)
			//
			// I am hard coding the first three specialized entry right now
			constexpr std::array spec_map{
				vk::SpecializationMapEntry().setConstantID(0).setOffset(0).setSize(
					sizeof(uint32_t)),
				vk::SpecializationMapEntry().setConstantID(1).setOffset(4).setSize(
					sizeof(uint32_t)),
				vk::SpecializationMapEntry().setConstantID(2).setOffset(8).setSize(
					sizeof(uint32_t)),
			};

			constexpr std::array spec_map_content{1u, 1u, 1u};

			const auto specialization_info =
				vk::SpecializationInfo().setMapEntries(spec_map).setData<uint32_t>(
					spec_map_content);

			const auto shader_stage_create_info =
				vk::PipelineShaderStageCreateInfo()
				.setStage(vk::ShaderStageFlagBits::eCompute)
				.setModule(handle_)
				.setPName("foo")
				.setPSpecializationInfo(&specialization_info);

			const auto pipeline_create_info = vk::ComputePipelineCreateInfo()
			                                  .setStage(shader_stage_create_info)
			                                  .setLayout(pipeline_layout_);
			if (const auto pipeline_result = device_ptr_->createComputePipeline(
					pipeline_cache_, pipeline_create_info);
				pipeline_result.result != vk::Result::eSuccess)
			{
				throw std::runtime_error("Cannot create compute pipeline");
			}
		}

		void create_parameters()
		{
			// How many parameters in the shader? usually 2: input and output
			constexpr auto num_parameters = 2u;

			// Pool size
			std::vector pool_sizes{
				vk::DescriptorPoolSize(
					vk::DescriptorType::eStorageBuffer, num_parameters)
			};

			// Descriptor pool
			const auto descriptor_pool_create_info =
				vk::DescriptorPoolCreateInfo().setMaxSets(1).setPoolSizes(pool_sizes);
			descriptor_pool_ =
				device_ptr_->createDescriptorPool(descriptor_pool_create_info);

			// Descriptor set
			std::vector<vk::DescriptorSetLayoutBinding> bindings;
			bindings.reserve(num_parameters);
			for (auto i = 0u; i < num_parameters; ++i)
			{
				bindings.emplace_back(i, // Binding index
				                      vk::DescriptorType::eStorageBuffer,
				                      1, // Descriptor count
				                      vk::ShaderStageFlagBits::eCompute);
			}
			const auto layout_create_info =
				vk::DescriptorSetLayoutCreateInfo().setBindings(bindings);
			descriptor_set_layout_ =
				device_ptr_->createDescriptorSetLayout(layout_create_info, nullptr);

			// Allocate descriptor set
			const auto set_alloc_info = vk::DescriptorSetAllocateInfo()
			                            .setDescriptorPool(descriptor_pool_)
			                            .setDescriptorSetCount(1)
			                            .setSetLayouts(descriptor_set_layout_);
			descriptor_set_ =
				device_ptr_->allocateDescriptorSets(set_alloc_info).front();

			// TODO: -------------------- tmp --------------
			std::array
				tmp_buffer{
					Buffer(device_ptr_, 1024),
					Buffer(device_ptr_, 1024)
				};

			const auto tmp_buffer_infos = std::array{
				vk::DescriptorBufferInfo()
				.setBuffer(tmp_buffer[0].get_handle())
				.setOffset(0)
				.setRange(tmp_buffer[0].get_size()),
				vk::DescriptorBufferInfo()
				.setBuffer(tmp_buffer[1].get_handle())
				.setOffset(0)
				.setRange(tmp_buffer[1].get_size()),
			};

			// TODO: -------------------- tmp --------------

			for (auto i = 0u; i < num_parameters; ++i)
			{
				// vk::DescriptorBufferInfo descriptorBufferInfo =
				//   this->mTensors[i]->constructDescriptorBufferInfo();
				const auto& buf_info = tmp_buffer_infos[i];

				std::array compute_write_descriptor_sets{
					vk::WriteDescriptorSet()
					.setDstSet(descriptor_set_)
					.setDstBinding(i)
					.setDstArrayElement(0)
					.setDescriptorCount(1)
					.setDescriptorType(vk::DescriptorType::eStorageBuffer)
					.setBufferInfo(buf_info)
				};

				device_ptr_->updateDescriptorSets(compute_write_descriptor_sets, nullptr);
			}
		}

		void destroy() override
		{
			device_ptr_->destroyPipeline(pipeline_);
			device_ptr_->destroyPipelineCache(pipeline_cache_);
			device_ptr_->destroyPipelineLayout(pipeline_layout_);
			device_ptr_->destroyDescriptorPool(descriptor_pool_);
			device_ptr_->destroyDescriptorSetLayout(descriptor_set_layout_);
		}

	private:
		vk::Pipeline pipeline_;
		vk::PipelineCache pipeline_cache_;
		vk::PipelineLayout pipeline_layout_;
		vk::DescriptorSetLayout descriptor_set_layout_;
		vk::DescriptorPool descriptor_pool_;
		vk::DescriptorSet descriptor_set_;

		void* m_specialization_constants_data_ = nullptr;
		uint32_t m_specialization_constants_data_type_memory_size_ = 0;
		uint32_t m_specialization_constants_size_ = 0;

		void* m_push_constants_data_ = nullptr;
		uint32_t m_push_constants_data_type_memory_size_ = 0;
		uint32_t m_push_constants_size_ = 0;

		Workgroup m_workgroup_;
	};
} // namespace core
