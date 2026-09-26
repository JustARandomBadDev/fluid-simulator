#include "graphics/vulkan/renderer.hpp"

#include <stdexcept>

#include "graphics/vulkan/device.hpp"
#include "graphics/vulkan/instance.hpp"
#include "graphics/vulkan/vulkan_buffer.hpp"

namespace fluid::graphics {

void Renderer::createCommandPool(Device &p_device, Instance &p_instance) {
    QueueFamilyIndices queueFamilyIndices =
        p_device.findQueueFamilies(p_device.getPhysicalDevice(), p_instance);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(
            p_device.getDevice(),
            &poolInfo,
            nullptr,
            &commandPool
        ) != VK_SUCCESS) {
        throw std::runtime_error(
            "Renderer::createCommandPool() -> failed to create graphics "
            "command pool"
        );
    }
}

void Renderer::createCommandBuffers(Device &p_device, uint32_t p_image_count) {
    if (!commandBuffers.empty()) {
        vkFreeCommandBuffers(
            p_device.getDevice(),
            commandPool,
            static_cast<uint32_t>(commandBuffers.size()),
            commandBuffers.data()
        );
        commandBuffers.clear();
    }

    swapchainImageCount = p_image_count;
    commandBuffers.resize(swapchainImageCount);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(
            p_device.getDevice(),
            &allocInfo,
            commandBuffers.data()
        ) != VK_SUCCESS) {
        commandBuffers.clear();
        throw std::runtime_error(
            "Renderer::createCommandBuffers() -> failed to allocate command "
            "buffers"
        );
    }

    currentFrame = framesInFlight == 0 ? 0 : (currentFrame % framesInFlight);

    if (copyCommandBuffer == VK_NULL_HANDLE) {
        VkCommandBufferAllocateInfo copyAllocInfo{};
        copyAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        copyAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        copyAllocInfo.commandPool = commandPool;
        copyAllocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(
                p_device.getDevice(),
                &copyAllocInfo,
                &copyCommandBuffer
            ) != VK_SUCCESS) {
            throw std::runtime_error(
                "Renderer::createCommandBuffers() -> failed to allocate copy "
                "command buffer"
            );
        }
    }
}

void Renderer::cleanupSyncObjects(Device &p_device) {
    if (p_device.getDevice() == VK_NULL_HANDLE) return;

    for (VkSemaphore semaphore : imageAvailableSemaphores) {
        if (semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(p_device.getDevice(), semaphore, nullptr);
        }
    }
    for (VkSemaphore semaphore : renderFinishedSemaphores) {
        if (semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(p_device.getDevice(), semaphore, nullptr);
        }
    }
    for (VkFence fence : inFlightFences) {
        if (fence != VK_NULL_HANDLE) {
            vkDestroyFence(p_device.getDevice(), fence, nullptr);
        }
    }

    imageAvailableSemaphores.clear();
    renderFinishedSemaphores.clear();
    inFlightFences.clear();
    imagesInFlight.clear();
}

void Renderer::createSyncObjects(
    Device &p_device,
    uint32_t p_frames_in_flight,
    uint32_t p_image_count
) {
    cleanupSyncObjects(p_device);

    framesInFlight = p_frames_in_flight;
    swapchainImageCount = p_image_count;
    imageAvailableSemaphores.resize(framesInFlight, VK_NULL_HANDLE);
    renderFinishedSemaphores.resize(swapchainImageCount, VK_NULL_HANDLE);
    inFlightFences.resize(framesInFlight, VK_NULL_HANDLE);
    imagesInFlight.assign(swapchainImageCount, VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    try {
        for (size_t i = 0; i < framesInFlight; i++) {
            if (vkCreateSemaphore(
                    p_device.getDevice(),
                    &semaphoreInfo,
                    nullptr,
                    &imageAvailableSemaphores[i]
                ) != VK_SUCCESS ||
                vkCreateFence(
                    p_device.getDevice(),
                    &fenceInfo,
                    nullptr,
                    &inFlightFences[i]
                ) != VK_SUCCESS) {
                throw std::runtime_error(
                    "Renderer::createSyncObjects() -> failed to create frame "
                    "synchronization objects"
                );
            }
        }

        for (size_t i = 0; i < swapchainImageCount; i++) {
            if (vkCreateSemaphore(
                    p_device.getDevice(),
                    &semaphoreInfo,
                    nullptr,
                    &renderFinishedSemaphores[i]
                ) != VK_SUCCESS) {
                throw std::runtime_error(
                    "Renderer::createSyncObjects() -> failed to create "
                    "render-finished semaphore"
                );
            }
        }
    } catch (...) {
        cleanupSyncObjects(p_device);
        framesInFlight = 0;
        swapchainImageCount = 0;
        throw;
    }
}

void Renderer::copyBuffer(
    const VulkanBuffer &p_source,
    const VulkanBuffer &p_destination,
    VkDeviceSize p_size,
    Device &p_device
) {
    if (copyCommandBuffer == VK_NULL_HANDLE) {
        throw std::runtime_error(
            "Renderer::copyBuffer() -> copy command buffer is not initialized"
        );
    }

    if (vkResetCommandBuffer(copyCommandBuffer, 0) != VK_SUCCESS) {
        throw std::runtime_error(
            "Renderer::copyBuffer() -> failed to reset copy command buffer"
        );
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(copyCommandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error(
            "Renderer::copyBuffer() -> failed to begin copy command buffer"
        );
    }

    VkBufferCopy copyRegion{};
    copyRegion.size = p_size;
    vkCmdCopyBuffer(
        copyCommandBuffer,
        p_source.getBuffer(),
        p_destination.getBuffer(),
        1,
        &copyRegion
    );

    if (vkEndCommandBuffer(copyCommandBuffer) != VK_SUCCESS) {
        throw std::runtime_error(
            "Renderer::copyBuffer() -> failed to record copy command buffer"
        );
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &copyCommandBuffer;

    if (vkQueueSubmit(
            p_device.getGraphicsQueue(),
            1,
            &submitInfo,
            VK_NULL_HANDLE
        ) != VK_SUCCESS) {
        throw std::runtime_error(
            "Renderer::copyBuffer() -> failed to submit buffer copy"
        );
    }
    if (vkQueueWaitIdle(p_device.getGraphicsQueue()) != VK_SUCCESS) {
        throw std::runtime_error(
            "Renderer::copyBuffer() -> failed while waiting for buffer copy"
        );
    }
}

void Renderer::cleanupFrameResources(Device &p_device) {
    cleanupSyncObjects(p_device);

    if (!commandBuffers.empty() && commandPool != VK_NULL_HANDLE &&
        p_device.getDevice() != VK_NULL_HANDLE) {
        vkFreeCommandBuffers(
            p_device.getDevice(),
            commandPool,
            static_cast<uint32_t>(commandBuffers.size()),
            commandBuffers.data()
        );
    }

    commandBuffers.clear();
    framesInFlight = 0;
    swapchainImageCount = 0;
    currentFrame = 0;
}

void Renderer::cleanup(Device &p_device) {
    cleanupFrameResources(p_device);

    if (commandPool != VK_NULL_HANDLE &&
        p_device.getDevice() != VK_NULL_HANDLE) {
        vkDestroyCommandPool(p_device.getDevice(), commandPool, nullptr);
    }

    commandPool = VK_NULL_HANDLE;
    copyCommandBuffer = VK_NULL_HANDLE;
}

void Renderer::incrementeCurrentFrame() {
    currentFrame = (currentFrame + 1) % framesInFlight;
}

} // namespace fluid::graphics
