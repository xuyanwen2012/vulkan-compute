#pragma once

#include <iostream>
#include <optional>
#include <spirv_cross/spirv_cross.hpp>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

#include "../../shaders/all_shaders.hpp"

namespace core
{
	class ComputeShader
	{
	public:
		struct BindMetaData
		{
			uint32_t set;
			uint32_t binding;
			uint32_t count;
			vk::DescriptorType descriptor_type;
			vk::ShaderStageFlags shader_stage_flag;
		};

		struct PushConstantMetaData
		{
			uint32_t size;
			uint32_t offset;
			vk::ShaderStageFlags shader_stage_flag;
		};

		ComputeShader(const std::shared_ptr<vk::Device>& device_ptr, std::string_view _)
			: device_ptr_(device_ptr)
		{
			// ------
			auto& shader_code = morton_spv;
			// ------

			const auto length = std::size(shader_code);
			const std::vector spivr_binary(shader_code, shader_code + length);

			CollectSpirvMetaData(spivr_binary, vk::ShaderStageFlagBits::eCompute);

			vk::ShaderModuleCreateInfo create_info;
			create_info.setPCode(spivr_binary.data())
			           .setCodeSize(spivr_binary.size() * sizeof(uint32_t));

			compute_shader_ = device_ptr->createShaderModule(create_info);
		}

		~ComputeShader()
		{
			// if (compute_shader) {
			// TODO: Bug here
			if (device_ptr_)
			{
				device_ptr_->destroyShaderModule(compute_shader_);
			}
			// }
		}

		friend std::ostream& operator<<(std::ostream& os, const BindMetaData& meta);
		friend std::ostream& operator<<(std::ostream& os, const PushConstantMetaData& meta);

	protected:
		void CollectSpirvMetaData(std::vector<uint32_t> spivr_binary,
		                          vk::ShaderStageFlagBits shader_flags);

		void GenerateVulkanDescriptorSetLayout();

		void GeneratePushConstantData();

	private:
		vk::ShaderModule compute_shader_;
		std::unordered_map<std::string, BindMetaData> reflection_datas_;
		std::vector<vk::DescriptorSetLayout> descriptor_set_layouts_;
		std::vector<vk::DescriptorSet> descriptor_sets_;

		std::optional<PushConstantMetaData> push_constant_meta_{std::nullopt};
		std::optional<vk::PushConstantRange> push_constant_range_{std::nullopt};

		// Handle
		std::shared_ptr<vk::Device> device_ptr_;

	public:
		vk::ShaderModule& get_handle() { return compute_shader_; }
	};

	inline std::ostream& operator<<(std::ostream& os,
	                                const ComputeShader::BindMetaData& meta)
	{
		os << "set: " << meta.set << '\n';
		os << "binding: " << meta.binding << '\n';
		os << "count: " << meta.count << '\n';
		// os << "descriptor type: " <<  meta.descriptor_type << '\n';
		// os << "shader stage flag: " << meta.shader_stage_flag << '\n';
		return os;
	}

	inline std::ostream& operator<<(std::ostream& os, const ComputeShader::PushConstantMetaData& meta)
	{
		os << "size: " << meta.size << '\n';
		os << "offset: " << meta.offset << '\n';
		// os << "shader stage flag: " << meta.shader_stage_flag << '\n';
		return os;
	}
} // namespace core
