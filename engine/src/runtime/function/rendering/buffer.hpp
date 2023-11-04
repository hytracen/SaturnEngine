#pragma once

#include <engine_pch.hpp>

#include "commands.hpp"
#include "device.hpp"
#include "image.hpp"

namespace saturn {

namespace rendering {

class Buffer {
public:
    Buffer(std::shared_ptr<Device> render_device, VkDeviceSize instance_size, uint32_t instance_count,
                 VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags memory_property_flags,
                 VkDeviceSize min_offset_alignment = 1);
    ~Buffer();

    Buffer(const Buffer &) = delete;
    auto operator=(const Buffer &) -> Buffer & = delete;

    auto Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) -> VkResult;
    void Unmap();

    void WriteToBuffer(void *data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    auto Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) -> VkResult;
    auto CreateDescriptorBufferInfo(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
            -> VkDescriptorBufferInfo;
    auto Invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) -> VkResult;

    void WriteToIndex(void *data, int index);
    auto FlushIndex(int index) -> VkResult;
    auto CreateDescriptorBufferInfoForIndex(int index) -> VkDescriptorBufferInfo;
    auto InvalidateIndex(int index) -> VkResult;

    void CopyToBuffer(std::shared_ptr<Buffer> target_buffer);
    void CopyToImage(std::shared_ptr<Image> target_image, uint32_t width, uint32_t height);
    void CopyToImage(VkImage target_vk_image, uint32_t width, uint32_t height);

    [[nodiscard]] auto GetVkBuffer() const -> VkBuffer { return m_buffer; }
    [[nodiscard]] auto GetMappedMemory() const -> void * { return m_mapped_memory; }
    [[nodiscard]] auto GetDeviceMemory() const -> VkDeviceMemory { return m_device_memory; }
    [[nodiscard]] auto GetInstanceCount() const -> uint32_t { return m_instance_count; }
    [[nodiscard]] auto GetInstanceSize() const -> VkDeviceSize { return m_instance_size; }
    [[nodiscard]] auto GetAlignmentSize() const -> VkDeviceSize { return m_instance_size; }
    [[nodiscard]] auto GetUsageFlags() const -> VkBufferUsageFlags { return usageFlags; }
    [[nodiscard]] auto GetMemoryPropertyFlags() const -> VkMemoryPropertyFlags { return memoryPropertyFlags; }
    [[nodiscard]] auto GetBufferSize() const -> VkDeviceSize { return m_buffer_size; }

private:
    static auto CalculateAlignment(VkDeviceSize instance_size, VkDeviceSize min_offset_alignment) -> VkDeviceSize;

    std::shared_ptr<Device> m_render_device;
    void *m_mapped_memory = nullptr;
    VkBuffer m_buffer;
    VkDeviceMemory m_device_memory = VK_NULL_HANDLE;

    VkDeviceSize m_buffer_size;
    uint32_t m_instance_count;
    VkDeviceSize m_instance_size;
    VkDeviceSize m_alignment_size;
    VkBufferUsageFlags usageFlags;
    VkMemoryPropertyFlags memoryPropertyFlags;
};

}// namespace rendering


}// namespace saturn