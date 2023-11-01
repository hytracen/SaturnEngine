#pragma once

#include <engine_pch.hpp>

#include "device.hpp"
#include "commands.hpp"

namespace saturn {

namespace rendering {

class Image {
public:
    Image(std::shared_ptr<Device> render_device, uint32_t w, uint32_t h,
                uint32_t mip_levels, VkSampleCountFlagBits num_samples, VkFormat format,
                VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);

    ~Image();

    void TransitionToLayout(VkImageLayout new_layout);

    [[nodiscard]] auto GetVkImage() -> VkImage { return m_image; }

private:
    std::shared_ptr<Device> m_device;
    VkImage m_image;
    VkDeviceMemory m_image_memory;
    VkImageLayout m_image_layout;
    uint32_t m_mip_levels;
};

}  // namespace rendering

}// namespace saturn