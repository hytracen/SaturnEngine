#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <engine_pch.hpp>
#include <runtime/function/rendering/buffer.hpp>
#include <runtime/function/rendering/commands.hpp>
#include <runtime/function/rendering/descriptor.hpp>
#include <runtime/function/rendering/device.hpp>
#include <runtime/function/rendering/image.hpp>
#include <runtime/function/rendering/pipeline.hpp>
#include <runtime/function/rendering/render_object.hpp>
#include <runtime/function/rendering/swapchain.hpp>
#include <runtime/function/rendering/window.hpp>
#include <runtime/resource/model.hpp>

namespace saturn {

namespace rendering {

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

class RenderSystem {
public:
    RenderSystem(uint32_t width, uint32_t height);
    ~RenderSystem();

    void Tick(float delta_time);
    
    auto ShouldCloseWindow() -> bool;

private:
    void Init();
    void Clear();

    void InitWindow();
    void InitVulkan();
    void InitImgui();

    void CreateDevice();
    void CreateSwapchain();
    void CreateDescriptorSetLayout();
    void CreateGraphicsPipeline();
    void CreateImage();
    void CreateImageSampler();
    void LoadModel();
    void CreateUniformBuffers();
    void CreateDescriptorPool();
    void CreateDescriptorSets();
    void CreateCommandBuffers();

    void RecreateSwapchain();
    void UpdateUniformBuffer(uint32_t current_frame_index);

    /**
     * @brief 开始录制command
     * 
     */
    void BeginFrame();

    void BeginRenderPass();

    void EndRenderPass();

    /**
     * @brief 结束录制command并提交
     * 
     */
    void EndFrame();

    std::shared_ptr<rendering::Window> m_window;
    std::shared_ptr<rendering::Device> m_render_device;
    std::unique_ptr<rendering::Swapchain> m_render_swapchain;
    std::shared_ptr<rendering::DescriptorPool> m_descriptor_pool;
    std::shared_ptr<rendering::DescriptorPool> m_imgui_descriptor_pool;
    std::shared_ptr<rendering::DescriptorSetLayout> m_descriptor_set_layout;
    std::shared_ptr<rendering::Image> m_render_image;
    std::vector<VkDescriptorSet> m_descriptor_sets;
    std::shared_ptr<rendering::Pipeline> m_graphics_pipeline;
    std::vector<std::shared_ptr<rendering::RenderObject>> m_render_objects;
    std::vector<std::shared_ptr<rendering::Buffer>> m_uniform_buffers;
    std::shared_ptr<rendering::CommandsBuilder> m_command_builder;

    VkSampler m_texture_sampler;
    uint32_t m_cur_swapchain_frame_index = 0;
    uint32_t m_image_index = 0;

    uint32_t m_width;
    uint32_t m_height;
};

}// namespace rendering

}// namespace saturn