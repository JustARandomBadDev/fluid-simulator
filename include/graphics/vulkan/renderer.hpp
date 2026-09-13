#ifndef FLUID_GRAPHICS_VULKAN_RENDERER_HPP
#define FLUID_GRAPHICS_VULKAN_RENDERER_HPP

#include <vulkan/vulkan.h>
#include <vector>

namespace fluid::graphics {

class Device;
class Instance;
class VulkanBuffer;

class Renderer {
public:
    void createCommandPool(Device& p_device, Instance& p_instance);
    void createCommandBuffers(Device& p_device, uint32_t p_image_count);
    void createSyncObjects(Device& p_device, uint32_t p_frames_in_flight, uint32_t p_image_count);
    void copyBuffer(const VulkanBuffer& p_source, const VulkanBuffer& p_destination, VkDeviceSize p_size, Device& p_device);
    void cleanupFrameResources(Device& p_device);
    void cleanup(Device& p_device);

    const VkCommandPool& getCommandPool() const { return commandPool; }
    const VkCommandBuffer& getCommandBuffer(uint32_t imageIndex) const { return commandBuffers[imageIndex]; }

    uint32_t getCurrentFrame() const { return currentFrame; }
    uint32_t getFramesInFlight() const { return framesInFlight; }
    uint32_t getSwapchainImageCount() const { return swapchainImageCount; }
    const VkFence& getCurrentInFlightFences() const { return inFlightFences[currentFrame]; }
    const VkFence& getImageInFlightFence(uint32_t imageIndex) const { return imagesInFlight[imageIndex]; }
    void setImageInFlightFence(uint32_t imageIndex, VkFence fence) { imagesInFlight[imageIndex] = fence; }

    const VkSemaphore& getCurrentImageAvailableSemaphores() const { return imageAvailableSemaphores[currentFrame]; }
    const VkSemaphore& getRenderFinishedSemaphore(uint32_t imageIndex) const { return renderFinishedSemaphores[imageIndex]; }

    void incrementeCurrentFrame();

private:
    VkCommandPool commandPool = VK_NULL_HANDLE;

    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;

    VkCommandBuffer copyCommandBuffer = VK_NULL_HANDLE;

    uint32_t framesInFlight = 0;
    uint32_t swapchainImageCount = 0;
    uint32_t currentFrame = 0;

    void cleanupSyncObjects(Device& p_device);
};

} // namespace fluid::graphics

#endif
