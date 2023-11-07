#include <iostream>
#include <optional>
#include <unordered_map>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include <spirv_cross/spirv_cross.hpp>
// #include <spirv_cross/spirv_cross_c.h>

#include "base_engine.hpp"
#include "file_reader.hpp"

#include <filesystem>
namespace fs = std::filesystem;

namespace core {
VmaAllocator allocator;
}

[[nodiscard]] std::ostream &operator<<(std::ostream &os, const glm::vec3 &v) {
  os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
  return os;
}

class Base {
public:
  Base(vkb::DispatchTable &disp) : disp{disp} {}

protected:
  vkb::DispatchTable &disp;
};

class ComputeShader : public Base {
public:
  struct BindMetaData {
    uint32_t set;
    uint32_t binding;
    uint32_t count;
    VkDescriptorType descriptor_type;
    VkShaderStageFlags shader_stage_flag;
  };

  struct PushConstantMetaData {
    uint32_t size;
    uint32_t offset;
    VkShaderStageFlags shader_stage_flag;
  };

  ComputeShader(vkb::DispatchTable &disp, std::string_view shader_path)
      : Base(disp) {
    auto &device = disp.device;
    // std::vector<VkDescriptorSetLayoutCreateInfo>
    // descriptorSetLayoutCreateInfos; std::vector<VkDescriptorSetLayoutBinding>
    // layoutBindings;

    auto spirvVertexBinary = io::ReadSpirvBinaryFile(shader_path);

    CollectSpirvMetaData(spirvVertexBinary,
                         VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT);
  }

  ~ComputeShader() {}

protected:
  void CollectSpirvMetaData(std::vector<uint32_t> spivrBinary,
                            const VkShaderStageFlagBits shaderFlags) {
    spirv_cross::Compiler compiler(std::move(spivrBinary));

    // spirv_cross::CompilerGLSL compiler(std::move(spivrBinary));
    // spirv_cross::ShaderResources resources = compiler.get_shader_resources();
  }

private:
  VkShaderModule compute_shader;
  std::unordered_map<std::string, BindMetaData> m_reflectionDatas;
  std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
  std::vector<VkDescriptorSet> m_descriptorSets;

  std::optional<PushConstantMetaData> m_pushConstantMeta{std::nullopt};
  std::optional<VkPushConstantRange> m_pushConstantRange{std::nullopt};
};

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <file_path>" << std::endl;
    return 1;
  }

  std::string relativePathString = argv[1];
  fs::path relativePath = relativePathString;
  fs::path absolutePath = fs::current_path() / relativePath;
  std::cout << "Absolute path: " << absolutePath << std::endl;

  if (!fs::exists(absolutePath)) {
    std::cerr << "Error: The specified file does not exist." << std::endl;
    return 1;
  }

  core::BaseEngine engine{};

  ComputeShader shader{engine.get_disp(), absolutePath.string()};

  return 0;
}
