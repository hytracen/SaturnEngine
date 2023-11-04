#pragma once

#include <engine_pch.hpp>

#include <vulkan/vulkan.h>

#include "commands.hpp"
#include "descriptor.hpp"
#include "device.hpp"


namespace saturn {

namespace rendering {

class Pipeline {
public:
    struct ConfigInfo {
        ConfigInfo();
        ConfigInfo(const ConfigInfo &) = delete;
        auto operator=(const ConfigInfo &) -> ConfigInfo & = delete;

        std::vector<VkVertexInputBindingDescription> m_binding_descriptions{};
        std::vector<VkVertexInputAttributeDescription> m_attribute_descriptions{};
        VkPipelineViewportStateCreateInfo m_viewport_info{};
        VkPipelineInputAssemblyStateCreateInfo m_input_assembly_info{};
        VkPipelineRasterizationStateCreateInfo m_rasterization_info{};
        VkPipelineMultisampleStateCreateInfo m_multisample_info{};
        VkPipelineColorBlendAttachmentState m_color_blend_attachment{};
        VkPipelineColorBlendStateCreateInfo m_color_blend_info{};
        VkPipelineDepthStencilStateCreateInfo m_depth_stencil_info{};
        std::vector<VkDynamicState> m_dynamic_state_enables{};
        VkPipelineDynamicStateCreateInfo m_dynamic_state_info{};
        VkPipelineLayout m_pipeline_layout = nullptr;
        VkRenderPass m_render_pass = nullptr;
        uint32_t m_subpass = 0;
    };

    class Builder {
    public:
        explicit Builder(std::shared_ptr<Device> device);
        auto EnableAlphaBlending() -> Builder &;
        auto BindShaders(std::string vert_shader_path, std::string frag_shader_path) -> Builder &;
        auto BindDescriptorSetLayout(std::shared_ptr<DescriptorSetLayout> descriptor_set_layout) -> Builder &;
        auto BindRenderpass(VkRenderPass render_pass) -> Builder &;
        auto SetMsaaSamples(VkSampleCountFlagBits sample_count) -> Builder &;
        auto Build() -> std::shared_ptr<Pipeline>;

    private:
        std::shared_ptr<Device> m_device;
        std::shared_ptr<ConfigInfo> m_config_info;
        std::string m_vert_path, m_frag_path;
    };

    Pipeline(std::shared_ptr<Device> device, const std::string &vert_filepath, const std::string &frag_filepath,
             std::shared_ptr<ConfigInfo> config_info);
    ~Pipeline();

    Pipeline(const Pipeline &) = delete;
    auto operator=(const Pipeline &) -> Pipeline & = delete;

    auto GetGraphicsPipeline() -> VkPipeline { return m_graphics_pipeline; };

    void CmdBindCommandBuffer(std::shared_ptr<CommandsBuilder> cmd_builder);
    void CmdBindDescriptorSets(std::shared_ptr<CommandsBuilder> cmd_builder, VkDescriptorSet descriptor_set) const;

private:
    void CreateGraphicsPipeline(const std::string &vert_filepath, const std::string &frag_filepath,
                                std::shared_ptr<ConfigInfo> config_info);

    void CreateShaderModule(const std::vector<char> &code, VkShaderModule *shader_module);

    std::shared_ptr<Device> m_device;
    std::shared_ptr<ConfigInfo> m_config_info;
    // ConfigInfo m_config_info;
    VkPipeline m_graphics_pipeline;
    VkShaderModule m_vert_shader_module;
    VkShaderModule m_frag_shader_module;
};

}// namespace rendering

}// namespace saturn