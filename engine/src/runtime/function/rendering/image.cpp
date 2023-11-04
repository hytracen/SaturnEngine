#include "image.hpp"
#include "buffer.hpp"

#include <stb_image.h>

namespace saturn {

namespace rendering {

Image::Image(std::shared_ptr<Device> render_device, uint32_t w, uint32_t h, uint32_t mip_levels,
             VkSampleCountFlagBits num_samples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
             VkMemoryPropertyFlags properties)
    : m_render_device(std::move(render_device)) {
    m_image_info.m_width = w;
    m_image_info.m_height = h;
    m_image_info.m_mip_levels = mip_levels;
    m_image_info.m_num_samples = num_samples;
    m_image_info.m_format = format;
    m_image_info.m_tiling = tiling;
    m_image_info.m_usage = usage;
    m_image_info.m_properties = properties;
    CreateImage();
}

Image::Image(const std::string &texture_path, std::shared_ptr<Device> render_device, VkSampleCountFlagBits num_samples,
             VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties)
    : m_render_device(std::move(render_device)) {

    m_image_info.m_num_samples = num_samples;
    m_image_info.m_format = format;
    m_image_info.m_tiling = tiling;
    m_image_info.m_usage = usage;
    m_image_info.m_properties = properties;

    int tex_width{};
    int tex_height{};
    int tex_channels{};

    stbi_uc *pixels =
            stbi_load((ENGINE_ROOT_DIR + texture_path).c_str(), &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);

    m_image_info.m_mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(tex_width, tex_height)))) + 1;
    m_image_info.m_width = tex_width;
    m_image_info.m_height = tex_height;

    if (!pixels) { throw std::runtime_error("Failed to load texture image!"); }

    Buffer staging_buffer{m_render_device, 4, static_cast<uint32_t>(tex_width * tex_height),
                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
    staging_buffer.Map();
    staging_buffer.WriteToBuffer(pixels);
    staging_buffer.Unmap();

    stbi_image_free(pixels);

    CreateImage();

    TransitionToLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    staging_buffer.CopyToImage(m_image, static_cast<uint32_t>(tex_width), static_cast<uint32_t>(tex_height));
    CreateMipmaps(m_image_info.m_mip_levels);
}

Image::~Image() {
    vkDestroyImage(m_render_device->GetVkDevice(), m_image, nullptr);
    vkDestroyImageView(m_render_device->GetVkDevice(), m_image_view, nullptr);

    vkFreeMemory(m_render_device->GetVkDevice(), m_image_memory, nullptr);
}

void Image::CreateImage() {

    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = m_image_info.m_width;
    image_info.extent.height = m_image_info.m_height;
    image_info.extent.depth = 1;
    image_info.mipLevels = m_image_info.m_mip_levels;
    image_info.arrayLayers = 1;
    image_info.format = m_image_info.m_format;
    image_info.tiling = m_image_info.m_tiling;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = m_image_info.m_usage;
    image_info.samples = m_image_info.m_num_samples;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    auto *vk_device = m_render_device->GetVkDevice();

    if (vkCreateImage(vk_device, &image_info, nullptr, &m_image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(vk_device, m_image, &mem_requirements);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex =
            m_render_device->FindMemoryType(mem_requirements.memoryTypeBits, m_image_info.m_properties);

    if (vkAllocateMemory(vk_device, &alloc_info, nullptr, &m_image_memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(vk_device, m_image, m_image_memory, 0);
}

void Image::TransitionToLayout(VkImageLayout new_layout) {
    CommandsBuilder cmd_builder{m_render_device};
    cmd_builder.AllocateCommandBuffers(1).BeginRecord();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = m_image_info.m_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = m_image_info.m_mip_levels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags source_stage;
    VkPipelineStageFlags destination_stage;

    if (m_image_info.m_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (m_image_info.m_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(cmd_builder.GetCurrentCommandBuffer(), source_stage, destination_stage, 0, 0, nullptr, 0,
                         nullptr, 1, &barrier);

    cmd_builder.EndRecord().SubmitTo(m_render_device->GetGraphicsQueue());
}

void Image::CreateMipmaps(uint32_t mip_levels) {
    // Check if image format supports linear blitting
    VkFormatProperties format_properties;
    vkGetPhysicalDeviceFormatProperties(m_render_device->GetPhyDevice(), m_image_info.m_format, &format_properties);

    if (!(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    rendering::CommandsBuilder cmd_builder{m_render_device};
    cmd_builder.AllocateCommandBuffers(1).BeginRecord();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = m_image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mip_width = m_image_info.m_width;
    int32_t mip_height = m_image_info.m_height;

    for (uint32_t i = 1; i < mip_levels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(cmd_builder.GetCurrentCommandBuffer(), VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mip_width, mip_height, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1};
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(cmd_builder.GetCurrentCommandBuffer(), m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_image,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd_builder.GetCurrentCommandBuffer(), VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        if (mip_width > 1) mip_width /= 2;
        if (mip_height > 1) mip_height /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mip_levels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd_builder.GetCurrentCommandBuffer(), VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    cmd_builder.EndRecord().SubmitTo(m_render_device->GetGraphicsQueue());
}

void Image::CreateImageView(VkImageAspectFlags aspect_flags) {
    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = m_image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = m_image_info.m_format;
    view_info.subresourceRange.aspectMask = aspect_flags;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = m_image_info.m_mip_levels;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_render_device->GetVkDevice(), &view_info, nullptr, &m_image_view) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image view!");
    }
}


}// namespace rendering

}// namespace saturn