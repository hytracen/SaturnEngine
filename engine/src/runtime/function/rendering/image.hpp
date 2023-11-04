#pragma once

#include <engine_pch.hpp>

#include "commands.hpp"
#include "device.hpp"


namespace saturn {

namespace rendering {

class Image {
public:
    struct Info {
        uint32_t m_width;
        uint32_t m_height;
        uint32_t m_mip_levels;
        VkSampleCountFlagBits m_num_samples;
        VkFormat m_format;
        VkImageTiling m_tiling;
        VkImageUsageFlags m_usage;
        VkMemoryPropertyFlags m_properties;
        VkImageLayout m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    };

    Image(std::shared_ptr<Device> render_device, uint32_t w, uint32_t h, uint32_t mip_levels,
          VkSampleCountFlagBits num_samples, VkFormat format, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
          VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                    VK_IMAGE_USAGE_SAMPLED_BIT,
          VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // 读取纹理
    Image(const std::string &texture_path, std::shared_ptr<Device> render_device, VkSampleCountFlagBits num_samples,
          VkFormat format, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
          VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                    VK_IMAGE_USAGE_SAMPLED_BIT,
          VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    ~Image();

    void TransitionToLayout(VkImageLayout new_layout);
    void CreateMipmaps(uint32_t mip_levels);
    void CreateImageView(VkImageAspectFlags aspect_flags);

    [[nodiscard]] auto GetVkImage() -> VkImage { return m_image; }
    [[nodiscard]] auto GetImageInfo() -> Info& { return m_image_info; }
    [[nodiscard]] auto GetVkImageView() -> VkImageView { return m_image_view; }

private:
    void CreateImage();

    std::shared_ptr<Device> m_render_device;
    VkImage m_image;
    VkDeviceMemory m_image_memory;
    VkImageView m_image_view;
    Info m_image_info;
};

}// namespace rendering

}// namespace saturn