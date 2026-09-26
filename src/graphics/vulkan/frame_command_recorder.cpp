#include "graphics/vulkan/command_recorder.hpp"

#include <array>
#include <stdexcept>

#include "graphics/vulkan/graphic_pipeline.hpp"
#include "graphics/vulkan/renderer.hpp"
#include "graphics/vulkan/swapchain.hpp"
#include "graphics/vulkan/vulkan_buffer.hpp"

namespace fluid::graphics {

CommandRecorder::CommandRecorder(
    Renderer &p_renderer,
    Swapchain &p_swapchain,
    GraphicPipeline &p_graphic_pipeline
)
    : _renderer(p_renderer), _swapchain(p_swapchain),
      _graphic_pipeline(p_graphic_pipeline) {}

void CommandRecorder::record(
    uint32_t p_image_index,
    const glm::vec4 &p_clear_color,
    const glm::mat4 &p_view_projection,
    const VulkanBuffer &p_particle_vertex_buffer,
    uint32_t p_vertex_count
) {
    const VkCommandBuffer command = _renderer.getCommandBuffer(p_image_index);

    if (vkResetCommandBuffer(command, 0) != VK_SUCCESS) {
        throw std::runtime_error(
            "FrameCommandRecorder::record() -> failed to reset command buffer"
        );
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(command, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error(
            "FrameCommandRecorder::record() -> failed to begin command buffer "
            "recording"
        );
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = _graphic_pipeline.getRenderPass();
    renderPassInfo.framebuffer =
        _swapchain.getSwapChainFramebuffers()[p_image_index];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = _swapchain.getSwapChainExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {
        {p_clear_color.r, p_clear_color.g, p_clear_color.b, p_clear_color.a}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(command, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(_swapchain.getSwapChainExtent().width);
    viewport.height =
        static_cast<float>(_swapchain.getSwapChainExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = _swapchain.getSwapChainExtent();
    vkCmdSetScissor(command, 0, 1, &scissor);

    vkCmdBindPipeline(
        command,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        _graphic_pipeline.getParticlePipeline()
    );

    const CameraPushConstants camera{p_view_projection};
    vkCmdPushConstants(
        command,
        _graphic_pipeline.getParticlePipelineLayout(),
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(camera),
        &camera
    );

    const VkBuffer vertexBuffers[] = {p_particle_vertex_buffer.getBuffer()};
    constexpr VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(command, 0, 1, vertexBuffers, offsets);
    vkCmdDraw(command, p_vertex_count, 1, 0, 0);

    vkCmdEndRenderPass(command);

    if (vkEndCommandBuffer(command) != VK_SUCCESS) {
        throw std::runtime_error(
            "FrameCommandRecorder::record() -> failed to record command buffer"
        );
    }
}

} // namespace fluid::graphics
