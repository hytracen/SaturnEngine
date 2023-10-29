#pragma once

#include <engine_pch.hpp>

#include "render_device.hpp"

namespace saturn {
class RenderBuffer {
public:
    RenderBuffer(
            std::shared_ptr<RenderDevice> render_device,
            VkDeviceSize instance_size,
            uint32_t instance_count,
            VkBufferUsageFlags usage_flags,
            VkMemoryPropertyFlags memory_property_flags,
            VkDeviceSize min_offset_alignment = 1);
    ~RenderBuffer();

    RenderBuffer(const RenderBuffer &) = delete;
    auto operator=(const RenderBuffer &) -> RenderBuffer & = delete;

    auto Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) -> VkResult;
    void Unmap();

    void WriteToBuffer(void *data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    auto Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) -> VkResult;
    auto CreateDescriptorBufferInfo(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) -> VkDescriptorBufferInfo;
    auto Invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) -> VkResult;

    void WriteToIndex(void *data, int index);
    auto FlushIndex(int index) -> VkResult;
    auto CreateDescriptorBufferInfoForIndex(int index) -> VkDescriptorBufferInfo;
    auto InvalidateIndex(int index) -> VkResult;

    [[nodiscard]] auto GetVkBuffer() const -> VkBuffer { return m_buffer; }
    [[nodiscard]] auto GetMappedMemory() const -> void * { return m_mapped_memory; }
    [[nodiscard]] auto GetDeviceMemory() const -> VkDeviceMemory { return m_device_memory; }
    [[nodiscard]] auto GetInstanceCount() const -> uint32_t { return instanceCount; }
    [[nodiscard]] auto GetInstanceSize() const -> VkDeviceSize { return m_instance_size; }
    [[nodiscard]] auto GetAlignmentSize() const -> VkDeviceSize { return m_instance_size; }
    [[nodiscard]] auto GetUsageFlags() const -> VkBufferUsageFlags { return usageFlags; }
    [[nodiscard]] auto GetMemoryPropertyFlags() const -> VkMemoryPropertyFlags { return memoryPropertyFlags; }
    [[nodiscard]] auto GetBufferSize() const -> VkDeviceSize { return m_buffer_size; }

private:
    static auto CalculateAlignment(VkDeviceSize instance_size, VkDeviceSize min_offset_alignment) -> VkDeviceSize;

    std::shared_ptr<RenderDevice> m_render_device;
    void *m_mapped_memory = nullptr;
    VkBuffer m_buffer;
    VkDeviceMemory m_device_memory = VK_NULL_HANDLE;

    VkDeviceSize m_buffer_size;
    uint32_t instanceCount;
    VkDeviceSize m_instance_size;
    VkDeviceSize m_alignment_size;
    VkBufferUsageFlags usageFlags;
    VkMemoryPropertyFlags memoryPropertyFlags;
};

}// namespace saturn