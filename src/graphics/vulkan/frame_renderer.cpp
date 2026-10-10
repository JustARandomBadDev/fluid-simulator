#include "graphics/vulkan/frame_renderer.hpp"

#include <stdexcept>

#include "core/camera.hpp"
#include "graphics/vulkan/command_recorder.hpp"
#include "graphics/vulkan/device.hpp"
#include "graphics/vulkan/renderer.hpp"
#include "graphics/vulkan/swapchain.hpp"

namespace fluid::graphics {

FrameRenderer::FrameRenderer(
    Device& p_device,
    Renderer& p_renderer,
    Swapchain& p_swapchain,
    CommandRecorder& p_command_recorder
)
    : _device(p_device), _renderer(p_renderer), _swapchain(p_swapchain),
      _command_recorder(p_command_recorder) {}

VkResult FrameRenderer::acquireFrameImage(uint32_t& p_image_index) {
    return vkAcquireNextImageKHR(
        _device.getDevice(),
        _swapchain.getSwapChain(),
        UINT64_MAX,
        _renderer.getCurrentImageAvailableSemaphores(),
        VK_NULL_HANDLE,
        &p_image_index
    );
}

void FrameRenderer::submitFrame(
    uint32_t p_image_index,
    VkSemaphore p_cuda_interop_semaphore,
    uint64_t p_cuda_ready_value,
    uint64_t p_vulkan_done_value
) {
    VkSemaphore waitSemaphores[] = {
        _renderer.getCurrentImageAvailableSemaphores(),
        p_cuda_interop_semaphore};
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT};
    const uint64_t waitValues[] = {0, p_cuda_ready_value};

    VkSemaphore signalSemaphores[] = {
        _renderer.getRenderFinishedSemaphore(p_image_index),
        p_cuda_interop_semaphore};
    const uint64_t signalValues[] = {0, p_vulkan_done_value};

    const bool useCudaInterop = p_cuda_interop_semaphore != VK_NULL_HANDLE;
    const uint32_t semaphoreCount = useCudaInterop ? 2U : 1U;

    if (vkResetFences(
            _device.getDevice(),
            1,
            &_renderer.getCurrentInFlightFences()
        ) != VK_SUCCESS) {
        throw std::runtime_error(
            "FrameRenderer::submitFrame() -> failed to reset frame fence"
        );
    }

    const VkCommandBuffer& commandBuffer =
        _renderer.getCommandBuffer(p_image_index);
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = semaphoreCount;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = semaphoreCount;
    submitInfo.pSignalSemaphores = signalSemaphores;

    VkTimelineSemaphoreSubmitInfo timelineInfo{};
    if (useCudaInterop) {
        timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
        timelineInfo.waitSemaphoreValueCount = semaphoreCount;
        timelineInfo.pWaitSemaphoreValues = waitValues;
        timelineInfo.signalSemaphoreValueCount = semaphoreCount;
        timelineInfo.pSignalSemaphoreValues = signalValues;
        submitInfo.pNext = &timelineInfo;
    }

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
    const core::Camera& camera,
    const glm::vec4& p_clear_color,
    VkBuffer p_particle_vertex_buffer,
    uint32_t p_vertex_count,
    VkSemaphore p_cuda_interop_semaphore,
    uint64_t p_cuda_ready_value,
    uint64_t p_vulkan_done_value
) {
    if (p_cuda_interop_semaphore != VK_NULL_HANDLE && !_frame_prepared) {
        throw std::runtime_error(
            "FrameRenderer::render() -> CUDA interop frame was not prepared"
        );
    }

    if (!_frame_prepared && !prepareFrame()) {
        return FrameRenderStatus::NeedsRecreate;
    }

    const uint32_t imageIndex = _prepared_image_index;
    const VkResult acquireResult = _prepared_acquire_result;

    _command_recorder.record(
        imageIndex,
        p_clear_color,
        camera.getProjectionMatrix() * camera.getViewMatrix(),
        p_particle_vertex_buffer,
        p_vertex_count
    );
    submitFrame(
        imageIndex,
        p_cuda_interop_semaphore,
        p_cuda_ready_value,
        p_vulkan_done_value
    );
    _frame_prepared = false;
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

bool FrameRenderer::prepareFrame() {
    if (_frame_prepared) return true;

    waitForCurrentFrame();

    uint32_t imageIndex = 0;
    const VkResult acquireResult = acquireFrameImage(imageIndex);
    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) return false;

    if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error(
            "FrameRenderer::prepareFrame() -> failed to acquire swapchain "
            "image"
        );
    }

    const VkFence& imageFence = _renderer.getImageInFlightFence(imageIndex);
    if (imageFence != VK_NULL_HANDLE && vkWaitForFences(
                                            _device.getDevice(),
                                            1,
                                            &imageFence,
                                            VK_TRUE,
                                            UINT64_MAX
                                        ) != VK_SUCCESS) {
        throw std::runtime_error(
            "FrameRenderer::prepareFrame() -> failed to wait for a swapchain "
            "image"
        );
    }
    _renderer.setImageInFlightFence(
        imageIndex,
        _renderer.getCurrentInFlightFences()
    );

    _prepared_image_index = imageIndex;
    _prepared_acquire_result = acquireResult;
    _frame_prepared = true;
    return true;
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
