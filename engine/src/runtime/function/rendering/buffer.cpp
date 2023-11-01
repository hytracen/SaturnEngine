#include "buffer.hpp"
// std
#include <cassert>
#include <cstring>

namespace saturn {

namespace rendering {

/**
 * 计算出每个实例满足设备最小偏移量的size，可以提高运算效率
 *
 * @param instance_size The size of an instance
 * @param min_offset_alignment The minimum required alignment, in bytes, for the offset member (eg
 * minUniformBufferOffsetAlignment)
 *
 * @return VkResult of the buffer mapping call
 */
auto Buffer::CalculateAlignment(VkDeviceSize instance_size, VkDeviceSize min_offset_alignment) -> VkDeviceSize {
    if (min_offset_alignment > 0) { return (instance_size + min_offset_alignment - 1) & ~(min_offset_alignment - 1); }
    return instance_size;
}

Buffer::Buffer(std::shared_ptr<Device> render_device, VkDeviceSize instance_size,
                           uint32_t instance_count, VkBufferUsageFlags usage_flags,
                           VkMemoryPropertyFlags memory_property_flags, VkDeviceSize min_offset_alignment)
    : m_render_device{std::move(render_device)}, m_instance_size{instance_size}, m_instance_count{instance_count},
      usageFlags{usage_flags}, memoryPropertyFlags{memory_property_flags} {

    m_alignment_size = CalculateAlignment(instance_size, min_offset_alignment);
    m_buffer_size = m_alignment_size * instance_count;
    m_render_device->CreateBuffer(m_buffer_size, usage_flags, memory_property_flags, m_buffer, m_device_memory);
}

Buffer::~Buffer() {
    Unmap();
    vkDestroyBuffer(m_render_device->GetVkDevice(), m_buffer, nullptr);
    vkFreeMemory(m_render_device->GetVkDevice(), m_device_memory, nullptr);
}

/**
 * 将一块内存绑定到此buffer上
 *
 * @param size (Optional) Size of the memory range to map. Pass VK_WHOLE_SIZE to map the complete
 * buffer range.
 * @param offset (Optional) Byte offset from beginning
 */
auto Buffer::Map(VkDeviceSize size, VkDeviceSize offset) -> VkResult {
    SATURN_ASSERT(m_buffer && m_device_memory, "Called map on buffer before create");
    return vkMapMemory(m_render_device->GetVkDevice(), m_device_memory, offset, size, 0, &m_mapped_memory);
}

/**
 * 解除绑定
 */
void Buffer::Unmap() {
    if (m_mapped_memory) {
        vkUnmapMemory(m_render_device->GetVkDevice(), m_device_memory);
        m_mapped_memory = nullptr;
    }
}

/**
 * 将数据写入buffer map的memory中
 *
 * @param data Pointer to the data to copy
 * @param size (Optional) Size of the data to copy. Pass VK_WHOLE_SIZE to flush the complete buffer
 * range.
 * @param offset (Optional) Byte offset from beginning of mapped region
 *
 */
void Buffer::WriteToBuffer(void *data, VkDeviceSize size, VkDeviceSize offset) {
    SATURN_ASSERT(m_mapped_memory, "Cannot copy to unmapped buffer");

    if (size == VK_WHOLE_SIZE) {
        memcpy(m_mapped_memory, data, m_buffer_size);
    } else {
        char *mem_offset = static_cast<char *>(m_mapped_memory);
        mem_offset += offset;
        memcpy(mem_offset, data, size);
    }
}

/**
 * 如果内存不具备VK_MEMORY_PROPERTY_HOST_COHERENT_BIT(一致性)这个能力的话，CPU与GPU的数据就无法及时更新入内存（存在缓存）。
 * 就要在CPU端读取之前，调用invalidate操作；在CPU写入数据后，进行flush操作。如果具备的话，则只需要map然后写入数据。
 */

/**
 * 使Buffer无效，以便Host能够访问
 *
 * @param size（可选）：Buffer的大小。传递 VK_WHOLE_SIZE 以使整个缓冲区范围无效。
 * @param offset（可选）：从Buffer起始位置的字节偏移量
 *
 * @return invalidate 调用的 VkResult
 */
auto Buffer::Invalidate(VkDeviceSize size, VkDeviceSize offset) -> VkResult {
    VkMappedMemoryRange mapped_range = {};
    mapped_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mapped_range.memory = m_device_memory;
    mapped_range.offset = offset;
    mapped_range.size = size;
    return vkInvalidateMappedMemoryRanges(m_render_device->GetVkDevice(), 1, &mapped_range);
}

/**
 * 刷新Buffer，使其对Device可见
 *
 * @param size（可选）：要刷新的内存范围的大小。传递 VK_WHOLE_SIZE 以刷新整个缓冲区范围。
 * @param offset（可选）：从Buffer起始位置的字节偏移量
 */
auto Buffer::Flush(VkDeviceSize size, VkDeviceSize offset) -> VkResult {
    VkMappedMemoryRange mapped_range = {};
    mapped_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mapped_range.memory = m_device_memory;
    mapped_range.offset = offset;
    mapped_range.size = size;
    return vkFlushMappedMemoryRanges(m_render_device->GetVkDevice(), 1, &mapped_range);
}

/**
 * Create a buffer info descriptor
 *
 * @param size (Optional) Size of the memory range of the descriptor
 * @param offset (Optional) Byte offset from beginning
 *
 * @return VkDescriptorBufferInfo of specified offset and range
 */
auto Buffer::CreateDescriptorBufferInfo(VkDeviceSize size, VkDeviceSize offset) -> VkDescriptorBufferInfo {
    return VkDescriptorBufferInfo{
            m_buffer,
            offset,
            size,
    };
}

/**
 * Copies "instanceSize" bytes of data to the mapped buffer at an offset of index * alignmentSize
 *
 * @param data Pointer to the data to copy
 * @param index Used in offset calculation
 *
 */
void Buffer::WriteToIndex(void *data, int index) {
    WriteToBuffer(data, m_instance_size, index * m_alignment_size);
}

/**
 *  Flush the memory range at index * alignmentSize of the buffer to make it visible to the device
 *
 * @param index Used in offset calculation
 *
 */
auto Buffer::FlushIndex(int index) -> VkResult { return Flush(m_alignment_size, index * m_alignment_size); }

/**
 * Create a buffer info descriptor
 *
 * @param index Specifies the region given by index * alignmentSize
 *
 * @return VkDescriptorBufferInfo for instance at index
 */
auto Buffer::CreateDescriptorBufferInfoForIndex(int index) -> VkDescriptorBufferInfo {
    return CreateDescriptorBufferInfo(m_alignment_size, index * m_alignment_size);
}

/**
 * Invalidate a memory range of the buffer to make it visible to the host
 *
 * @note Only required for non-coherent memory
 *
 * @param index Specifies the region to invalidate: index * alignmentSize
 *
 * @return VkResult of the invalidate call
 */
auto Buffer::InvalidateIndex(int index) -> VkResult {
    return Invalidate(m_alignment_size, index * m_alignment_size);
}

void Buffer::CopyToBuffer(std::shared_ptr<Buffer> target_buffer) {
    CommandsBuilder cmd_builder{m_render_device};
    cmd_builder.AllocateCommandBuffers(1).BeginRecord();

    VkBufferCopy copy_region{};
    copy_region.size = m_buffer_size;
    vkCmdCopyBuffer(cmd_builder.GetCurrentCommandBuffer(), m_buffer, target_buffer->GetVkBuffer(), 1, &copy_region);

    cmd_builder.EndRecord().SubmitTo(m_render_device->GetGraphicsQueue());
}

void Buffer::CopyToImage(std::shared_ptr<Image> target_image, uint32_t width, uint32_t height) {
    CommandsBuilder cmd_buffer_builder{m_render_device};
    cmd_buffer_builder.AllocateCommandBuffers(1).BeginRecord();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    // 指定数据被复制到图像的哪一部分
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(cmd_buffer_builder.GetCurrentCommandBuffer(), m_buffer, target_image->GetVkImage(),
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    cmd_buffer_builder.EndRecord().SubmitTo(m_render_device->GetGraphicsQueue());
}

}// namespace rendering


}// namespace saturn