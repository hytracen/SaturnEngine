#pragma once

#include <engine_pch.hpp>

#include "window.hpp"

namespace saturn {

namespace rendering {

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
    std::optional<uint32_t> m_graphics_family;
    std::optional<uint32_t> m_present_family;
    [[nodiscard]] auto IsComplete() const -> bool {
        return m_graphics_family.has_value() && m_present_family.has_value();
    }
};

class Device {
public:
    explicit Device(const std::string &engine_name, const std::string &game_name, std::shared_ptr<Window> window);
    ~Device();

    void CreateInstance(const std::string &engine_name, const std::string &game_name);
    void CreateSurface();

    //----------------------Getter----------------------
    [[nodiscard]] auto GetVkInstance() -> VkInstance { return m_vk_instance; };
    [[nodiscard]] auto IsEnableValidationLayers() const -> bool { return m_enable_validation_layers; };
    [[nodiscard]] auto GetValidationLayers() const -> std::vector<const char *> { return m_validation_layers; }
    auto GetCommandPool() -> VkCommandPool { return m_command_pool; }
    auto GetPhyDevice() -> VkPhysicalDevice { return m_physical_device; }
    auto GetVkDevice() -> VkDevice { return m_device; }
    auto GetGraphicsQueue() -> VkQueue { return m_graphics_queue; }
    auto GetPresentQueue() -> VkQueue { return m_present_queue; }
    auto GetMaxMsaaSamples() -> VkSampleCountFlagBits { return m_msaa_samples_flag; }
    auto GetRenderWindow() -> std::shared_ptr<Window> { return m_render_window; }
    auto GetSurface() -> VkSurfaceKHR { return m_surface; }
    //--------------------------------------------------

    //---------------------Image------------------------
    auto CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels)
            -> VkImageView;
    void CreateImage(uint32_t width, uint32_t height, uint32_t mip_levels, VkSampleCountFlagBits num_samples,
                     VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                     VkImage &image, VkDeviceMemory &image_memory);
    //--------------------------------------------------

    // Buffer Helper Functions
    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer,
                      VkDeviceMemory &buffer_memory);

    void CopyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);

    void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layer_count);

    void CreateImageWithInfo(const VkImageCreateInfo &image_info, VkMemoryPropertyFlags properties, VkImage &image,
                             VkDeviceMemory &image_memory);

    auto FindPhysicalQueueFamilies() -> QueueFamilyIndices { return FindQueueFamilies(m_physical_device); }
    auto FindMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties) -> uint32_t;

private:
    // Not copyable or movable
    Device(const Device &) = delete;
    auto operator=(const Device &) -> Device & = delete;
    Device(Device &&) = delete;
    auto operator=(Device &&) -> Device & = delete;

    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateCommandPool();

    auto GetMaxUsableSampleCount() -> VkSampleCountFlagBits;
    auto IsValidationLayerSupport() -> bool;

    // helper functions
    [[nodiscard]] auto GetRequiredExtensions() const -> std::vector<const char *>;
    auto IsPhyDeviceSuitable(VkPhysicalDevice device) -> bool;
    auto FindQueueFamilies(VkPhysicalDevice device) -> QueueFamilyIndices;
    auto CheckDeviceExtensionSupport(VkPhysicalDevice device) -> bool;
    auto QuerySwapChainSupport(VkPhysicalDevice device) -> SwapChainSupportDetails;
    auto GetSwapChainSupport() -> SwapChainSupportDetails { return QuerySwapChainSupport(m_physical_device); }
    auto FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                             VkFormatFeatureFlags features) -> VkFormat;

    VkInstance m_vk_instance;
    VkDebugUtilsMessengerEXT m_debug_messenger;
    std::vector<const char *> m_validation_layers{"VK_LAYER_KHRONOS_validation"};

    std::shared_ptr<Window> m_render_window;
    VkSampleCountFlagBits m_msaa_samples_flag = VK_SAMPLE_COUNT_1_BIT;// 最大支持的采样数
    VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;

    VkCommandPool m_command_pool;
    VkSurfaceKHR m_surface;

    VkDevice m_device;
    VkSurfaceKHR surface_;
    VkQueue m_graphics_queue;
    VkQueue m_present_queue;

#ifdef SATURN_DEBUG
    const bool m_enable_validation_layers = true;
#else
    const bool m_enable_validation_layers = false;
#endif

    const std::vector<const char *> m_device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
};

auto CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *p_create_info,
                                  const VkAllocationCallbacks *p_allocator, VkDebugUtilsMessengerEXT *p_debug_messenger)
        -> VkResult;
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger,
                                   const VkAllocationCallbacks *p_allocator);


}// namespace rendering

}// namespace saturn
