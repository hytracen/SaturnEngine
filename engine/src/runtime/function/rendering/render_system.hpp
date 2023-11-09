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
    alignas(16) glm::mat4 light_view_proj;
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
    void CreateShadowmapPipeline();
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
     */
    void BeginFrame();

    /**
     * @brief 结束录制command并提交
     */
    void EndFrame();

    void BeginOffscreenRenderPass();

    void EndOffscreenRenderPass();

    /**
     * @brief 最终将物体渲染到屏幕上的pass
     */
    void BeginShadingRenderPass();

    void EndShadingRenderPass();

    std::shared_ptr<Window> m_window;
    std::shared_ptr<Device> m_render_device;
    std::unique_ptr<Swapchain> m_render_swapchain;

    std::shared_ptr<DescriptorPool> m_descriptor_pool;
    std::shared_ptr<DescriptorPool> m_imgui_descriptor_pool;
    std::shared_ptr<DescriptorSetLayout> m_shadowmap_descriptor_set_layout;
    std::shared_ptr<DescriptorSetLayout> m_descriptor_set_layout;
    std::vector<VkDescriptorSet> m_shadowmap_descriptor_sets;
    std::vector<VkDescriptorSet> m_descriptor_sets;

    std::shared_ptr<Image> m_render_image;

    std::shared_ptr<Pipeline> m_shading_pipeline;
    std::shared_ptr<Pipeline> m_shadowmap_pipeline;

    std::vector<std::shared_ptr<RenderObject>> m_render_objects;
    std::vector<std::shared_ptr<Buffer>> m_uniform_buffers;
    std::shared_ptr<CommandsBuilder> m_command_builder;

    VkSampler m_texture_sampler;
    uint32_t m_cur_swapchain_frame_index = 0;
    uint32_t m_image_index = 0;

    uint32_t m_width;
    uint32_t m_height;

    uint32_t m_test = 0;
};

}// namespace rendering

}// namespace saturn