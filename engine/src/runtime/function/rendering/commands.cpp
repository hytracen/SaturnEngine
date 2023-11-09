#include "commands.hpp"

namespace saturn {

namespace rendering {

CommandsBuilder::CommandsBuilder(std::shared_ptr<Device> device)
    : m_render_device(std::move(device)) {}

CommandsBuilder::~CommandsBuilder() {
    for (auto *command_buffer: m_command_buffers) {
        vkFreeCommandBuffers(m_render_device->GetVkDevice(), m_render_device->GetCommandPool(), 1, &command_buffer);
    }
}

auto CommandsBuilder::AllocateCommandBuffers(int count) -> CommandsBuilder & {
    m_command_buffers.resize(count);
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = m_render_device->GetCommandPool();
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = count;

    if (vkAllocateCommandBuffers(m_render_device->GetVkDevice(), &alloc_info, m_command_buffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers!");
    }

    return *this;
}

auto CommandsBuilder::SetCurrentCommandBuffer(int index) -> CommandsBuilder & {
    m_current_command_buffer_index = index;
    return *this;
}

auto CommandsBuilder::BeginRecord() -> CommandsBuilder & {
    auto *command_buffer = GetCurrentCommandBuffer();
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer, &begin_info);
    return *this;
}

auto CommandsBuilder::EndRecord() -> CommandsBuilder & {
    auto *command_buffer = GetCurrentCommandBuffer();
    vkEndCommandBuffer(command_buffer);
    return *this;
}

auto CommandsBuilder::WaitForSemaphoresAndStages(const std::vector<VkSemaphore> &semaphores,
                                                       const std::vector<VkPipelineStageFlags> &stages)
        -> CommandsBuilder & {
    m_wait_for_semaphores = semaphores;
    m_wait_for_stages = stages;
    return *this;
}

auto CommandsBuilder::SignalSemaphores(const std::vector<VkSemaphore> &semaphores) -> CommandsBuilder & {
    m_signal_semaphores = semaphores;
    return *this;
}

auto CommandsBuilder::SignalFence(VkFence fence) -> CommandsBuilder & {
    m_fence = fence;
    return *this;
}

auto CommandsBuilder::SubmitTo(VkQueue queue) -> CommandsBuilder & {
    auto *command_buffer = GetCurrentCommandBuffer();
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    if (!m_wait_for_semaphores.empty() && !m_wait_for_stages.empty()) {
        submit_info.waitSemaphoreCount = m_wait_for_semaphores.size();
        submit_info.pWaitSemaphores = m_wait_for_semaphores.data();
        submit_info.pWaitDstStageMask = m_wait_for_stages.data();
    }

    if (!m_signal_semaphores.empty()) {
        submit_info.signalSemaphoreCount = m_signal_semaphores.size();
        submit_info.pSignalSemaphores = m_signal_semaphores.data();
    }

    vkQueueSubmit(queue, 1, &submit_info, m_fence);

    if (m_wait_for_semaphores.empty()) { vkQueueWaitIdle(queue); }

    // reset
    m_wait_for_semaphores.clear();
    m_signal_semaphores.clear();
    m_wait_for_stages.clear();
    m_fence = VK_NULL_HANDLE;
    m_current_command_buffer_index = 0;

    return *this;
}

auto CommandsBuilder::GetCurrentCommandBuffer() -> VkCommandBuffer {
    return m_command_buffers.at(m_current_command_buffer_index);
}

}  // namespace rendering

}// namespace saturn