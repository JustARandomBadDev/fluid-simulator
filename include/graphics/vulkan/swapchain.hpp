#ifndef FLUID_GRAPHICS_VULKAN_SWAPCHAIN_HPP
#define FLUID_GRAPHICS_VULKAN_SWAPCHAIN_HPP

#include <vector>

#include <vulkan/vulkan.h>

namespace fluid::graphics {

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class Device;
class GraphicPipeline;
class Instance;

class Swapchain {
  public:
    void createSwapChain(
        VkExtent2D p_framebuffer_extent,
        Instance &p_instance,
        Device &p_device,
        uint32_t p_frames_in_flight
    );
    void createImageViews(Device &p_device);
    void
    createFramebuffers(GraphicPipeline &p_graphic_pipeline, Device &p_device);
    void cleanupFramebuffers(Device &p_device);
    void cleanup(Device &p_device);

    SwapChainSupportDetails
    querySwapChainSupport(VkPhysicalDevice pdevice, Instance &p_instance) const;
    VkImageView createImageView(
        VkImage image,
        VkFormat format,
        VkImageAspectFlags aspectFlags,
        Device &p_device
    ) const;

    const VkSwapchainKHR &getSwapChain() const {
        return swapChain;
    }
    const VkFormat &getSwapChainImageFormat() const {
        return swapChainImageFormat;
    }
    const VkExtent2D &getSwapChainExtent() const {
        return swapChainExtent;
    }
    const std::vector<VkImageView> &getSwapChainImageViews() const {
        return swapChainImageViews;
    }
    const std::vector<VkFramebuffer> &getSwapChainFramebuffers() const {
        return swapChainFramebuffers;
    }
    const VkFramebuffer &getFramebuffers(uint32_t imageIndex) const {
        return swapChainFramebuffers[imageIndex];
    }

    float getAspectRatio() const {
        return static_cast<float>(swapChainExtent.width) /
               static_cast<float>(swapChainExtent.height);
    }
    uint32_t getImageCount() const {
        return imageCount;
    }

  private:
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D swapChainExtent{};
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    uint32_t imageCount = 0;

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR> &availableFormats
    );
    VkPresentModeKHR chooseSwapPresentMode(
        const std::vector<VkPresentModeKHR> &availablePresentModes
    );
    VkExtent2D chooseSwapExtent(
        VkExtent2D p_framebuffer_extent,
        const VkSurfaceCapabilitiesKHR &capabilities
    );
    VkCompositeAlphaFlagBitsKHR
    chooseCompositeAlpha(const VkSurfaceCapabilitiesKHR &capabilities);
};

} // namespace fluid::graphics

#endif
