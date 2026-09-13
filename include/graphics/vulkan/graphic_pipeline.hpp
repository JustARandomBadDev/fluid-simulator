#ifndef FLUID_GRAPHICS_VULKAN_GRAPHIC_PIPELINE_HPP
#define FLUID_GRAPHICS_VULKAN_GRAPHIC_PIPELINE_HPP

#include <filesystem>
#include <vector>

#include <glm/mat4x4.hpp>
#include <vulkan/vulkan.h>

namespace fluid::graphics {

class Device;
class Swapchain;

struct CameraPushConstants {
    glm::mat4 viewProjection{1.0F};
};

class GraphicPipeline {
public:
    void createRenderPass(Swapchain& p_swapchain, Device& p_device);
    void createGraphicsPipeline(
        const std::filesystem::path& vertex_shader_path,
        const std::filesystem::path& fragment_shader_path,
        Device& p_device
    );
    void cleanup(Device& p_device);

    VkShaderModule createShaderModule(const std::vector<char>& code, Device& p_device);
    static std::vector<char> readFile(const std::filesystem::path& filename);

    VkRenderPass getRenderPass() const { return renderPass; }
    const VkPipeline& getParticlePipeline() const { return particlePipeline; }
    const VkPipelineLayout& getParticlePipelineLayout() const { return particlePipelineLayout; }

private:
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipelineLayout particlePipelineLayout = VK_NULL_HANDLE;
    VkPipeline particlePipeline = VK_NULL_HANDLE;
};

} // namespace fluid::graphics

#endif
