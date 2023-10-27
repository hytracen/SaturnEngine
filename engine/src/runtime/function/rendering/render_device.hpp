#pragma once

#include <engine_pch.hpp>

#include "render_window.hpp"

namespace saturn {

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
    std::optional<uint32_t> m_graphics_family;
    std::optional<uint32_t> m_present_family;
    [[nodiscard]] auto IsComplete() const -> bool { return m_graphics_family.has_value() && m_present_family.has_value(); }
};

class RenderDevice {
public:
    explicit RenderDevice(const std::string& engine_name, const std::string &game_name, std::shared_ptr<RenderWindow> window);
    ~RenderDevice();

    void CreateInstance(const std::string& engine_name, const std::string& game_name);
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
    auto GetRenderWindow() -> std::shared_ptr<RenderWindow> { return m_render_window; }
    auto GetSurface() -> VkSurfaceKHR { return m_surface; }
    //--------------------------------------------------

    auto FindPhysicalQueueFamilies() -> QueueFamilyIndices { return FindQueueFamilies(m_physical_device); }

private:
    // Not copyable or movable
    RenderDevice(const RenderDevice &) = delete;
    auto operator=(const RenderDevice &) -> RenderDevice & = delete;
    RenderDevice(RenderDevice &&) = delete;
    auto operator=(RenderDevice &&) -> RenderDevice & = delete;

    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateCommandPool();

    // Buffer Helper Functions
    void CreateBuffer(
            VkDeviceSize size,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags properties,
            VkBuffer &buffer,
            VkDeviceMemory &buffer_memory);
    auto BeginSingleTimeCommands() -> VkCommandBuffer;
    void EndSingleTimeCommands(VkCommandBuffer command_buffer);
    void CopyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);
    void CopyBufferToImage(
            VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layer_count);

    void CreateImageWithInfo(
            const VkImageCreateInfo &image_info,
            VkMemoryPropertyFlags properties,
            VkImage &image,
            VkDeviceMemory &image_memory);

    VkPhysicalDeviceProperties properties;

    auto GetMaxUsableSampleCount() -> VkSampleCountFlagBits;
    auto IsValidationLayerSupport() -> bool;

    // helper functions
    [[nodiscard]] auto GetRequiredExtensions() const -> std::vector<const char *>;
    auto IsPhyDeviceSuitable(VkPhysicalDevice device) -> bool;
    auto FindQueueFamilies(VkPhysicalDevice device) -> QueueFamilyIndices;
    auto CheckDeviceExtensionSupport(VkPhysicalDevice device) -> bool;
    auto QuerySwapChainSupport(VkPhysicalDevice device) -> SwapChainSupportDetails;
    auto GetSwapChainSupport() -> SwapChainSupportDetails { return QuerySwapChainSupport(m_physical_device); }
    auto FindMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties) -> uint32_t;
    auto FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) -> VkFormat;

    VkInstance m_vk_instance;
    VkDebugUtilsMessengerEXT m_debug_messenger;
    std::vector<const char *> m_validation_layers{"VK_LAYER_KHRONOS_validation"};

    // std::shared_ptr<RenderInstance> m_render_ins;
    std::shared_ptr<RenderWindow> m_render_window;
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

auto CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *p_create_info, const VkAllocationCallbacks *p_allocator, VkDebugUtilsMessengerEXT *p_debug_messenger) -> VkResult;
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger, const VkAllocationCallbacks *p_allocator);

}// namespace saturn
