#pragma once

#include <engine_pch.hpp>

#include "device.hpp"
#include <vulkan/vulkan.h>


namespace saturn {

namespace rendering {

enum class RenderCommandsUsage {
    OneTimeSubmit,// 只会提交一次，不会复用
};

class CommandsBuilder {
public:
    explicit CommandsBuilder(std::shared_ptr<Device> device);
    ~CommandsBuilder();

    auto AllocateCommandBuffers(int count) -> CommandsBuilder &;
    auto SetCurrentCommandBuffer(int index) -> CommandsBuilder &;
    auto BeginRecord() -> CommandsBuilder &;
    auto EndRecord() -> CommandsBuilder &;
    auto WaitForSemaphoresAndStages(const std::vector<VkSemaphore> &semaphores,
                                    const std::vector<VkPipelineStageFlags> &stages) -> CommandsBuilder &;
    auto SignalSemaphores(const std::vector<VkSemaphore> &semaphores) -> CommandsBuilder &;
    auto SignalFence(VkFence fence) -> CommandsBuilder &;

    auto SubmitTo(VkQueue queue) -> CommandsBuilder &;

    auto GetCurrentCommandBuffer() -> VkCommandBuffer;

private:
    std::shared_ptr<Device> m_render_device;
    int m_current_command_buffer_index{0};
    std::vector<VkCommandBuffer> m_command_buffers;
    std::vector<VkSemaphore> m_wait_for_semaphores, m_signal_semaphores;
    std::vector<VkPipelineStageFlags> m_wait_for_stages;
    VkFence m_fence {VK_NULL_HANDLE};
};

}



}// namespace saturn