#include "image.hpp"

namespace saturn {

namespace rendering {

Image::Image(std::shared_ptr<Device> render_device, uint32_t w, uint32_t h,
                         uint32_t mip_levels, VkSampleCountFlagBits num_samples, VkFormat format,
                         VkImageTiling tiling, VkImageUsageFlags usage,
                         VkMemoryPropertyFlags properties)
    : m_device(std::move(render_device)), m_mip_levels(mip_levels),
      m_image_layout(VK_IMAGE_LAYOUT_UNDEFINED) {
    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = w;
    image_info.extent.height = h;
    image_info.extent.depth = 1;
    image_info.mipLevels = mip_levels;
    image_info.arrayLayers = 1;
    image_info.format = format;
    image_info.tiling = tiling;
    image_info.initialLayout = m_image_layout;
    image_info.usage = usage;
    image_info.samples = num_samples;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    m_device->CreateImageWithInfo(image_info, properties, m_image, m_image_memory);
}

Image::~Image() {
    vkDestroyImage(m_device->GetVkDevice(), m_image, nullptr);
    vkFreeMemory(m_device->GetVkDevice(), m_image_memory, nullptr);
}

void Image::TransitionToLayout(VkImageLayout new_layout) {
    CommandsBuilder cmd_builder{m_device};
    cmd_builder.AllocateCommandBuffers(1).BeginRecord();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = m_image_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = m_mip_levels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags source_stage;
    VkPipelineStageFlags destination_stage;

    if (m_image_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
        new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (m_image_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(cmd_builder.GetCurrentCommandBuffer(), source_stage, destination_stage, 0,
                         0, nullptr, 0, nullptr, 1, &barrier);

    cmd_builder.EndRecord().SubmitTo(m_device->GetGraphicsQueue());
}

}  // namespace rendering

}// namespace saturn