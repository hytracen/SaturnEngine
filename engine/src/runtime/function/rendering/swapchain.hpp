#include <engine_pch.hpp>

#include "device.hpp"
#include "image.hpp"

namespace saturn {

namespace rendering {

class Swapchain {
public:
    explicit Swapchain(std::shared_ptr<Device> render_device);
    explicit Swapchain(std::shared_ptr<Device> render_device, std::shared_ptr<Swapchain> old_swapchain);
    ~Swapchain();

    auto GetImageAvailableSemaphores() -> std::vector<VkSemaphore> & { return m_image_available_semaphores; }
    auto GetRenderFinishedSemaphores() -> std::vector<VkSemaphore> & { return m_render_finished_semaphores; }
    auto GetInFlightFences() -> std::vector<VkFence> & { return m_in_flight_fences; }

    auto GetOffscreenRenderPass() -> VkRenderPass { return m_offscreen_renderpass; }
    auto GetShadingRenderPass() -> VkRenderPass { return m_shading_renderpass; }
    [[nodiscard]] auto GetFramebuffer() -> std::vector<VkFramebuffer>& { return m_framebuffer; }
    [[nodiscard]] auto GetOffscreenFramebuffer() -> std::vector<VkFramebuffer>& { return m_offscreen_framebuffer; }
    [[nodiscard]] auto GetMaxFramesInFlight() const -> int { return m_max_frames_inflight; }

    auto GetShadowmapImage() -> std::shared_ptr<Image> { return m_shadowmap_image; }

    auto VkSwapchain() -> VkSwapchainKHR { return m_vk_swapchain; }
    auto Extent() -> VkExtent2D { return m_swapchain_extent; }

    auto AcquireNextImage(uint32_t swapchain_frame_index) -> std::pair<VkResult, uint32_t>;

private:
    void Init();

    void CreateSwapchain();
    void CreateImageViews();
    void CreateShadingRenderPass();
    void CreateOffscreenRenderPass();
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
    
    std::shared_ptr<Device> m_device;
    VkSwapchainKHR m_vk_swapchain;
    std::shared_ptr<Swapchain> m_old_swapchain;

    std::vector<VkImage> m_swapchain_images; // 最终渲染在屏幕上的图像，开启MSAA时，用于resolve（解析采样后的图像）
    std::vector<VkImageView> m_swapchain_imageviews;

    std::shared_ptr<Image> m_color_image;
    std::shared_ptr<Image> m_shadowmap_image;
    std::shared_ptr<Image> m_depth_image;

    VkFormat m_swapchain_image_format;
    VkExtent2D m_swapchain_extent;
    std::vector<VkFramebuffer> m_offscreen_framebuffer;
    std::vector<VkFramebuffer> m_framebuffer;
    VkRenderPass m_shading_renderpass;
    VkRenderPass m_offscreen_renderpass;

    std::vector<VkSemaphore> m_image_available_semaphores;
    std::vector<VkSemaphore> m_render_finished_semaphores;
    std::vector<VkFence> m_in_flight_fences;

    const int m_max_frames_inflight = 2;
};

}  // namespace rendering

}// namespace saturn