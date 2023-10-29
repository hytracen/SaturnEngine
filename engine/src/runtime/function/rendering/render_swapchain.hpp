#include <engine_pch.hpp>

#include "render_device.hpp"

namespace saturn {

class RenderSwapchain {
public:
    explicit RenderSwapchain(std::shared_ptr<RenderDevice> render_device);
    explicit RenderSwapchain(std::shared_ptr<RenderDevice> render_device, std::shared_ptr<RenderSwapchain> old_swapchain);
    ~RenderSwapchain();

    auto ImageAvailableSemaphores() -> std::vector<VkSemaphore> & { return m_image_available_semaphores; }
    auto RenderFinishedSemaphores() -> std::vector<VkSemaphore> & { return m_render_finished_semaphores; }
    auto InFlightFences() -> std::vector<VkFence> & { return m_in_flight_fences; }

    auto GetRenderPass() -> VkRenderPass { return m_renderpass; }

    auto ColorImage() -> VkImage { return m_color_image; }
    auto ColorImageMemory() -> VkDeviceMemory { return m_color_image_memory; }
    auto ColorImageView() -> VkImageView { return m_color_imageview; }

    auto DepthImage() -> VkImage { return m_depth_image; }
    auto DepthImageMemory() -> VkDeviceMemory { return m_depth_image_memory; }
    auto DepthImageView() -> VkImageView { return m_depth_imageview; }

    auto VkSwapchain() -> VkSwapchainKHR { return m_swapchain; }
    auto Extent() -> VkExtent2D { return m_swapchain_extent; }

    auto GetFramebuffer() -> std::vector<VkFramebuffer>& { return m_framebuffer; }

    const int m_max_frames_inflight = 2;

private:
    void CreateSwapchain();
    void CreateImageViews();
    void CreateRenderPass();
    void CreateColorResources();
    void CreateDepthResources();
    void CreateFramebuffers();
    void CreateSyncObjects();

    auto QuerySwapChainSupport(VkPhysicalDevice device) -> SwapChainSupportDetails;
    auto ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &available_formats) -> VkSurfaceFormatKHR;
    auto ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &available_present_modes) -> VkPresentModeKHR;
    auto ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) -> VkExtent2D;
    auto FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) -> VkFormat;
    auto FindDepthFormat() -> VkFormat;
    

    std::shared_ptr<RenderDevice> m_render_device;
    VkSwapchainKHR m_swapchain;
    std::shared_ptr<RenderSwapchain> m_old_swapchain;
    std::vector<VkImage> m_swapchain_images;

    VkImage m_color_image;
    VkDeviceMemory m_color_image_memory;
    VkImageView m_color_imageview;

    VkImage m_depth_image;
    VkDeviceMemory m_depth_image_memory;
    VkImageView m_depth_imageview;

    VkFormat m_swapchain_image_format;
    VkExtent2D m_swapchain_extent;
    std::vector<VkImageView> m_swapchain_imageviews;
    std::vector<VkFramebuffer> m_framebuffer;
    VkRenderPass m_renderpass;

    std::vector<VkSemaphore> m_image_available_semaphores;
    std::vector<VkSemaphore> m_render_finished_semaphores;
    std::vector<VkFence> m_in_flight_fences;
};

}// namespace saturn