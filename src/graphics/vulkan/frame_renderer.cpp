#include "graphics/vulkan/frame_renderer.hpp"

#include <stdexcept>

#include "core/camera.hpp"
#include "graphics/vulkan/command_recorder.hpp"
#include "graphics/vulkan/device.hpp"
#include "graphics/vulkan/renderer.hpp"
#include "graphics/vulkan/swapchain.hpp"
#include "graphics/vulkan/vulkan_buffer.hpp"

namespace fluid::graphics {

FrameRenderer::FrameRenderer(
    Device &p_device,
    Renderer &p_renderer,
    Swapchain &p_swapchain,
    CommandRecorder &p_command_recorder
)
    : _device(p_device), _renderer(p_renderer), _swapchain(p_swapchain),
      _command_recorder(p_command_recorder) {}

VkResult FrameRenderer::acquireFrameImage(uint32_t &p_image_index) {
    return vkAcquireNextImageKHR(
        _device.getDevice(),
        _swapchain.getSwapChain(),
        UINT64_MAX,
        _renderer.getCurrentImageAvailableSemaphores(),
        VK_NULL_HANDLE,
        &p_image_index
    );
}

void FrameRenderer::submitFrame(uint32_t p_image_index) {
    VkSemaphore waitSemaphores[] = {
        _renderer.getCurrentImageAvailableSemaphores()};
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {
        _renderer.getRenderFinishedSemaphore(p_image_index)};

    if (vkResetFences(
            _device.getDevice(),
            1,
            &_renderer.getCurrentInFlightFences()
        ) != VK_SUCCESS) {
        throw std::runtime_error(
            "FrameRenderer::submitFrame() -> failed to reset frame fence"
        );
    }

    const VkCommandBuffer &commandBuffer =
        _renderer.getCommandBuffer(p_image_index);
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(
            _device.getGraphicsQueue(),
            1,
            &submitInfo,
            _renderer.getCurrentInFlightFences()
        ) != VK_SUCCESS) {
        throw std::runtime_error(
            "FrameRenderer::submitFrame() -> failed to submit draw command "
            "buffer"
        );
    }
}

VkResult FrameRenderer::presentFrame(uint32_t p_image_index) {
    VkSemaphore signalSemaphores[] = {
        _renderer.getRenderFinishedSemaphore(p_image_index)};
    VkSwapchainKHR swapChains[] = {_swapchain.getSwapChain()};

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &p_image_index;

    return vkQueuePresentKHR(_device.getPresentQueue(), &presentInfo);
}

FrameRenderStatus FrameRenderer::render(
    const core::Camera &camera,
    const glm::vec4 &p_clear_color,
    const VulkanBuffer &p_particle_vertex_buffer,
    uint32_t p_vertex_count
) {
    waitForCurrentFrame();

    uint32_t imageIndex = 0;
    const VkResult acquireResult = acquireFrameImage(imageIndex);
    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        return FrameRenderStatus::NeedsRecreate;
    }

    if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error(
            "FrameRenderer::render() -> failed to acquire swapchain image"
        );
    }

    const VkFence &imageFence = _renderer.getImageInFlightFence(imageIndex);
    if (imageFence != VK_NULL_HANDLE && vkWaitForFences(
                                            _device.getDevice(),
                                            1,
                                            &imageFence,
                                            VK_TRUE,
                                            UINT64_MAX
                                        ) != VK_SUCCESS) {
        throw std::runtime_error(
            "FrameRenderer::render() -> failed to wait for a swapchain image"
        );
    }
    _renderer.setImageInFlightFence(
        imageIndex,
        _renderer.getCurrentInFlightFences()
    );

    _command_recorder.record(
        imageIndex,
        p_clear_color,
        camera.getProjectionMatrix() * camera.getViewMatrix(),
        p_particle_vertex_buffer,
        p_vertex_count
    );
    submitFrame(imageIndex);
    _renderer.incrementeCurrentFrame();

    const VkResult presentResult = presentFrame(imageIndex);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR ||
        presentResult == VK_SUBOPTIMAL_KHR ||
        acquireResult == VK_SUBOPTIMAL_KHR) {
        return FrameRenderStatus::NeedsRecreate;
    }

    if (presentResult != VK_SUCCESS) {
        throw std::runtime_error(
            "FrameRenderer::render() -> failed to present swapchain image"
        );
    }

    return FrameRenderStatus::Rendered;
}

void FrameRenderer::waitForCurrentFrame() {
    if (vkWaitForFences(
            _device.getDevice(),
            1,
            &_renderer.getCurrentInFlightFences(),
            VK_TRUE,
            UINT64_MAX
        ) != VK_SUCCESS) {
        throw std::runtime_error(
            "FrameRenderer::waitForCurrentFrame() -> failed to wait for the "
            "current frame"
        );
    }
}

} // namespace fluid::graphics
