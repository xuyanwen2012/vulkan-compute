#pragma once

#include "vulkan_resource.hpp"

namespace core {

class Tensor : public VulkanResource<vk::Buffer> {
public:
  Tensor(std::shared_ptr<vk::Device> device_ptr, void *data,
         const uint32_t elementTotalCount, const uint32_t elementMemorySize)
      : VulkanResource(std::move(device_ptr)) {
    rebuild(data, elementTotalCount, elementMemorySize);
  }

  ~Tensor() override { destroy(); }

  void rebuild(void *data, const uint32_t elementTotalCount,
               const uint32_t elementMemorySize) {
    data_size_ = elementTotalCount;
    data_type_memory_size_ = elementMemorySize;
    raw_data_ = data;
  }

  // getters // setters

  // total number of elements
  [[nodiscard]] constexpr uint32_t data_size() const { return data_size_; }

  // total size of a single element of the respective data type
  [[nodiscard]] constexpr uint32_t data_memory_size() const {
    return data_type_memory_size_ * data_size_;
  }

  [[nodiscard]] void *raw_data() { return raw_data_; }

  template <typename T> [[nodiscard]] T *data() {
    return static_cast<T *>(raw_data_);
  }

  template <typename T> [[nodiscard]] std::vector<T> vector() const {
    return {data<T>(), data<T>() + data_size()};
  }

  void set_raw_data(const void *data) {
    std::memcpy(raw_data_, data, data_memory_size());
  }

  // used by Vulkan API
  [[nodiscard]] auto construct_descriptor_buffer_info() const {
    return vk::DescriptorBufferInfo()
        .setBuffer(get_handle())
        .setOffset(0)
        .setRange(data_memory_size());
  }

protected:
  void destroy() override {
    if (handle_ != VK_NULL_HANDLE) {
    }
  }

  void create_buffer() {}

  void allocate_bind_memory() {}

protected:
  uint32_t data_size_;             // N
  uint32_t data_type_memory_size_; // sizeof(T)
  void *raw_data_;
};

template <typename T> class TensorT final : public Tensor {
public:
  TensorT(std::shared_ptr<vk::Device> device_ptr, void *data,
          const uint32_t elementTotalCount)
      : Tensor(std::move(device_ptr), data, elementTotalCount, sizeof(T)) {}

  ~TensorT() override = default;

  T *data() { return (T *)this->mRawData; }

  [[nodiscard]] std::vector<T> vector() const { return vector<T>(); }

  T &operator[](int index) { return *(((T *)raw_data_) + index); }

  void set_data(const std::vector<T> &data) {

    if (data.size() != data_size_) {
      throw std::runtime_error(
          "Kompute TensorT Cannot set data of different sizes");
    }

    Tensor::set_raw_data(data.data());
  }
};

} // namespace core
