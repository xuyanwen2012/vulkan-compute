#pragma once

#include <iostream>
#include <memory>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>

#include "VkBootstrap.h"
#include "vk_mem_alloc.h"

namespace core {
// Need a global allocator for VMA
extern VmaAllocator g_allocator;

// Base Engine does The Setup. Single device, single queue
class BaseEngine {
public:
  BaseEngine() {
    try {
      device_initialization();
      get_queues();
      vma_initialization();
    } catch (std::exception &e) {
      std::cout << e.what() << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  ~BaseEngine() {
    spdlog::debug("BaseEngine::~BaseEngine");
    destory();
  }

  void destory() {
    if (g_allocator != VK_NULL_HANDLE) {
      vmaDestroyAllocator(g_allocator);
    }
    destroy_device(device_);
    destroy_instance(instance_);
  }

  [[nodiscard]] vkb::Device &get_device() { return device_; }

  [[nodiscard]] auto get_device_ptr() {
    return std::make_shared<vk::Device>(device_.device);
  }

private:
  void device_initialization() {
    // Vulkan instance creation (1/3)
    vkb::InstanceBuilder instance_builder;
    auto inst_ret = instance_builder.set_app_name("Example Vulkan Application")
                        .request_validation_layers()
                        .use_default_debug_messenger()
                        .require_api_version(1, 3, 0) // for SPIR-V 1.3
                        .build();
    if (!inst_ret) {
      std::cerr << "Failed to create Vulkan instance. Error: "
                << inst_ret.error().message() << "\n";
      throw std::runtime_error("Failed to create Vulkan instance");
    }

    instance_ = inst_ret.value();

    // Vulkan pick physical device (2/3)
    vkb::PhysicalDeviceSelector selector{instance_};
    auto phys_ret =
        selector.defer_surface_initialization()
            .set_minimum_version(1, 2)
            .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
            //.prefer_gpu_device_type(vkb::PreferredDeviceType::integrated)
            .allow_any_gpu_device_type(false)
            .select();

    if (!phys_ret) {
      std::cerr << "Failed to select Vulkan Physical Device. Error: "
                << phys_ret.error().message() << "\n";
      throw std::runtime_error("Failed to select Vulkan Physical Device");
    }

    spdlog::info("selected GPU: {}", phys_ret.value().properties.deviceName);

    // Vulkan logical device creation (3/3)
    vkb::DeviceBuilder device_builder{phys_ret.value()};
    auto dev_ret = device_builder.build();
    if (!dev_ret) {
      std::cerr << "Failed to create Vulkan device. Error: "
                << dev_ret.error().message() << "\n";
      throw std::runtime_error("Failed to create Vulkan device");
    }

    device_ = dev_ret.value();
  }

  void get_queues() {
    auto q_ret = device_.get_queue(vkb::QueueType::compute);
    if (!q_ret.has_value()) {
      throw std::runtime_error("Failed to get compute queue");
    }
    queue_ = q_ret.value();
  }

  void vma_initialization() const {
    if (g_allocator != VK_NULL_HANDLE) {
      return;
    }

    const VmaAllocatorCreateInfo allocator_create_info{
        .physicalDevice = device_.physical_device,
        .device = device_.device,
        .instance = instance_.instance,
    };

    vmaCreateAllocator(&allocator_create_info, &g_allocator);
  }

protected:
  vkb::Instance instance_;
  vkb::Device device_;
  vk::Queue queue_;
};
} // namespace core
