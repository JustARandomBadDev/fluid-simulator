#ifndef FLUID_GRAPHICS_VULKAN_INSTANCE_HPP
#define FLUID_GRAPHICS_VULKAN_INSTANCE_HPP

#include <functional>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

namespace fluid::graphics {

class Instance {
public:
    void createInstance(bool p_enable_validation_layers, const std::vector<std::string>& p_required_extensions);
    void setupDebugMessenger();
    void createSurface(const std::function<VkResult(VkInstance, VkSurfaceKHR&)>& p_create_surface);
    void cleanup();

    const VkInstance& getInstance() const { return instance; }
    const VkSurfaceKHR& getSurface() const { return surface; }

private:
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    bool _validation_layers_enabled = false;

    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions(const std::vector<std::string>& p_required_extensions) const;
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    );
};

} // namespace fluid::graphics

#endif
