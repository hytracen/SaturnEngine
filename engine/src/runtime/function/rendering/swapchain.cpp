#include "swapchain.hpp"

#include <utility>

namespace saturn {

namespace rendering {

Swapchain::Swapchain(std::shared_ptr<Device> render_device) : m_device(std::move(render_device)) { Init(); }

Swapchain::Swapchain(std::shared_ptr<Device> render_device, std::shared_ptr<Swapchain> old_swapchain)
    : m_device(std::move(render_device)), m_old_swapchain(std::move(old_swapchain)) {
    Init();
    m_old_swapchain.reset();
}

void Swapchain::Init() {
    CreateSwapchain();
    CreateImageViews();
    CreateShadingRenderPass();
    CreateOffscreenRenderPass();
    CreateColorResources();
    CreateDepthResources();
    CreateFramebuffers();
    CreateSyncObjects();
}

Swapchain::~Swapchain() {
    for (auto *framebuffer: m_framebuffer) {
        vkDestroyFramebuffer(m_device->GetVkDevice(), framebuffer, nullptr);
    }

    for (auto *offscreen_framebuffer: m_offscreen_framebuffer) {
        vkDestroyFramebuffer(m_device->GetVkDevice(), offscreen_framebuffer, nullptr);
    }

    for (auto *image_view: m_swapchain_imageviews) {
        vkDestroyImageView(m_device->GetVkDevice(), image_view, nullptr);
    }

    for (size_t i = 0; i < m_max_frames_inflight; i++) {
        vkDestroySemaphore(m_device->GetVkDevice(), m_render_finished_semaphores[i], nullptr);
        vkDestroySemaphore(m_device->GetVkDevice(), m_image_available_semaphores[i], nullptr);
        vkDestroyFence(m_device->GetVkDevice(), m_in_flight_fences[i], nullptr);
    }

    vkDestroyRenderPass(m_device->GetVkDevice(), m_shading_renderpass, nullptr);
    vkDestroyRenderPass(m_device->GetVkDevice(), m_offscreen_renderpass, nullptr);

    vkDestroySwapchainKHR(m_device->GetVkDevice(), m_vk_swapchain, nullptr);
}

auto Swapchain::AcquireNextImage(uint32_t swapchain_frame_index) -> std::pair<VkResult, uint32_t> {
    uint32_t image_index;
    VkResult result =
            vkAcquireNextImageKHR(m_device->GetVkDevice(), m_vk_swapchain, UINT64_MAX,
                                  m_image_available_semaphores[swapchain_frame_index], VK_NULL_HANDLE, &image_index);
    return {result, image_index};
}

void Swapchain::CreateSwapchain() {
    SwapChainSupportDetails swap_chain_support = QuerySwapChainSupport(m_device->GetPhyDevice());

    VkSurfaceFormatKHR surface_format = ChooseSwapSurfaceFormat(swap_chain_support.formats);
    VkPresentModeKHR present_mode = ChooseSwapPresentMode(swap_chain_support.presentModes);
    VkExtent2D extent = ChooseSwapExtent(swap_chain_support.capabilities);

    uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
    if (swap_chain_support.capabilities.maxImageCount > 0 &&
        image_count > swap_chain_support.capabilities.maxImageCount) {
        image_count = swap_chain_support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = m_device->GetSurface();

    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = m_device->FindPhysicalQueueFamilies();
    uint32_t queue_family_indices[] = {indices.m_graphics_family.value(), indices.m_present_family.value()};

    if (indices.m_graphics_family != indices.m_present_family) {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    create_info.preTransform = swap_chain_support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = m_old_swapchain == nullptr ? VK_NULL_HANDLE : m_old_swapchain->VkSwapchain();

    if (vkCreateSwapchainKHR(m_device->GetVkDevice(), &create_info, nullptr, &m_vk_swapchain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(m_device->GetVkDevice(), m_vk_swapchain, &image_count, nullptr);
    m_swapchain_images.resize(image_count);
    vkGetSwapchainImagesKHR(m_device->GetVkDevice(), m_vk_swapchain, &image_count, m_swapchain_images.data());

    m_swapchain_image_format = surface_format.format;
    m_swapchain_extent = extent;
}

void Swapchain::CreateImageViews() {
    m_swapchain_imageviews.resize(m_swapchain_images.size());

    for (uint32_t i = 0; i < m_swapchain_images.size(); i++) {
        m_swapchain_imageviews[i] = m_device->CreateImageView(m_swapchain_images[i], m_swapchain_image_format,
                                                                     VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}

void Swapchain::CreateShadingRenderPass() {
    VkAttachmentDescription color_attachment{};
    color_attachment.format = m_swapchain_image_format;
    color_attachment.samples = m_device->GetMaxMsaaSamples();
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depth_attachment{};
    depth_attachment.format = FindDepthFormat();
    depth_attachment.samples = m_device->GetMaxMsaaSamples();
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription color_attachment_resolve{};
    color_attachment_resolve.format = m_swapchain_image_format;
    color_attachment_resolve.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment_resolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment_resolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment_resolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment_resolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment_resolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment_resolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_ref{};
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference color_attachment_resolve_ref{};
    color_attachment_resolve_ref.attachment = 2;
    color_attachment_resolve_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;
    subpass.pResolveAttachments = &color_attachment_resolve_ref;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 3> attachments = {color_attachment, depth_attachment, color_attachment_resolve};
    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    render_pass_info.pAttachments = attachments.data();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    if (vkCreateRenderPass(m_device->GetVkDevice(), &render_pass_info, nullptr, &m_shading_renderpass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

void Swapchain::CreateOffscreenRenderPass() {
    VkAttachmentDescription depth_attachment{};
    depth_attachment.format = FindDepthFormat();
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference depth_attachment_ref{};
    depth_attachment_ref.attachment = 0;
    depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 1> attachments = {depth_attachment};
    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    render_pass_info.pAttachments = attachments.data();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    if (vkCreateRenderPass(m_device->GetVkDevice(), &render_pass_info, nullptr, &m_offscreen_renderpass) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

void Swapchain::CreateDepthResources() {
    VkFormat depth_format = FindDepthFormat();

    {
        m_depth_image = std::make_shared<Image>(m_device, m_swapchain_extent.width, m_swapchain_extent.height, 1,
                                                m_device->GetMaxMsaaSamples(), depth_format,
                                                VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_depth_image->CreateImageView(VK_IMAGE_ASPECT_DEPTH_BIT);
    }

    {
        m_shadowmap_image =
                std::make_shared<Image>(m_device, 4096, 4096, 1,
                                        VK_SAMPLE_COUNT_1_BIT, depth_format, VK_IMAGE_TILING_OPTIMAL,
                                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_shadowmap_image->CreateImageView(VK_IMAGE_ASPECT_DEPTH_BIT);
    }
}

void Swapchain::CreateColorResources() {
    VkFormat color_format = m_swapchain_image_format;

    m_color_image =
            std::make_shared<Image>(m_device, m_swapchain_extent.width, m_swapchain_extent.height, 1,
                                    m_device->GetMaxMsaaSamples(), color_format, VK_IMAGE_TILING_OPTIMAL,
                                    VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    m_color_image->CreateImageView(VK_IMAGE_ASPECT_COLOR_BIT);
}

void Swapchain::CreateFramebuffers() {
    // shadowmap
    {
        m_offscreen_framebuffer.resize(m_swapchain_imageviews.size());

        for (size_t i = 0; i < m_swapchain_imageviews.size(); i++) {
            std::vector<VkImageView> attachments = {m_shadowmap_image->GetVkImageView()};

            VkFramebufferCreateInfo framebuffer_info{};
            framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebuffer_info.renderPass = m_offscreen_renderpass;
            framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebuffer_info.pAttachments = attachments.data();
            framebuffer_info.width = m_shadowmap_image->GetImageInfo().m_width;
            framebuffer_info.height = m_shadowmap_image->GetImageInfo().m_height;
            framebuffer_info.layers = 1;

            if (vkCreateFramebuffer(m_device->GetVkDevice(), &framebuffer_info, nullptr,
                                    &m_offscreen_framebuffer[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    // shading
    {
        m_framebuffer.resize(m_swapchain_imageviews.size());

        for (size_t i = 0; i < m_swapchain_imageviews.size(); i++) {
            std::vector<VkImageView> attachments = {m_color_image->GetVkImageView(), m_depth_image->GetVkImageView(),
                                                    m_swapchain_imageviews[i]};

            VkFramebufferCreateInfo framebuffer_info{};
            framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebuffer_info.renderPass = m_shading_renderpass;
            framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebuffer_info.pAttachments = attachments.data();
            framebuffer_info.width = m_swapchain_extent.width;
            framebuffer_info.height = m_swapchain_extent.height;
            framebuffer_info.layers = 1;

            if (vkCreateFramebuffer(m_device->GetVkDevice(), &framebuffer_info, nullptr, &m_framebuffer[i]) !=
                VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }
}

void Swapchain::CreateSyncObjects() {
    m_image_available_semaphores.resize(m_max_frames_inflight);
    m_render_finished_semaphores.resize(m_max_frames_inflight);
    m_in_flight_fences.resize(m_max_frames_inflight);

    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < m_max_frames_inflight; i++) {
        if (vkCreateSemaphore(m_device->GetVkDevice(), &semaphore_info, nullptr,
                              &m_image_available_semaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_device->GetVkDevice(), &semaphore_info, nullptr,
                              &m_render_finished_semaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_device->GetVkDevice(), &fence_info, nullptr, &m_in_flight_fences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

auto Swapchain::QuerySwapChainSupport(VkPhysicalDevice device) -> SwapChainSupportDetails {
    SwapChainSupportDetails details;

    auto *surface = m_device->GetSurface();

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);

    if (format_count != 0) {
        details.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
    }

    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);

    if (present_mode_count != 0) {
        details.presentModes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, details.presentModes.data());
    }

    return details;
}

auto Swapchain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &available_formats)
        -> VkSurfaceFormatKHR {
    for (const auto &available_format: available_formats) {
        if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return available_format;
        }
    }

    return available_formats[0];
}

auto Swapchain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &available_present_modes)
        -> VkPresentModeKHR {
    for (const auto &available_present_mode: available_present_modes) {
        if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {// 三重缓冲
            return available_present_mode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

auto Swapchain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) -> VkExtent2D {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) { return capabilities.currentExtent; }
    int width{};
    int height{};
    glfwGetFramebufferSize(m_device->GetRenderWindow()->GetGlfwWindow(), &width, &height);

    VkExtent2D actual_extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

    actual_extent.width =
            std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actual_extent.height =
            std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actual_extent;
}

auto Swapchain::FindDepthFormat() -> VkFormat {
    return FindSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                               VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

auto Swapchain::FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                                    VkFormatFeatureFlags features) -> VkFormat {
    for (VkFormat format: candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_device->GetPhyDevice(), format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) { return format; }
        if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

}// namespace rendering

}// namespace saturn