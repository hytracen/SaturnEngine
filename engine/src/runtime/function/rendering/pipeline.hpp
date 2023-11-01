#pragma once

#include <engine_pch.hpp>

#include "device.hpp"
#include <vulkan/vulkan.h>


namespace saturn {

namespace rendering {

class Pipeline {
public:
    struct ConfigInfo {
        ConfigInfo();
        ConfigInfo(const ConfigInfo &) = delete;
        auto operator=(const ConfigInfo &) -> ConfigInfo & = delete;

        std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
        VkPipelineViewportStateCreateInfo viewportInfo;
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
        VkPipelineRasterizationStateCreateInfo rasterizationInfo;
        VkPipelineMultisampleStateCreateInfo multisampleInfo;
        VkPipelineColorBlendAttachmentState colorBlendAttachment;
        VkPipelineColorBlendStateCreateInfo colorBlendInfo;
        VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
        std::vector<VkDynamicState> dynamicStateEnables;
        VkPipelineDynamicStateCreateInfo dynamicStateInfo;
        VkPipelineLayout pipelineLayout = nullptr;
        VkRenderPass renderPass = nullptr;
        uint32_t subpass = 0;
    };

    Pipeline(std::shared_ptr<Device> device, const std::string &vert_filepath,
                   const std::string &frag_filepath, const ConfigInfo &config_info);
    ~Pipeline();

    Pipeline(const Pipeline &) = delete;
    auto operator=(const Pipeline &) -> Pipeline & = delete;

    void Bind(VkCommandBuffer command_buffer);

    static void DefaultPipelineConfigInfo(ConfigInfo &config_info);
    static void EnableAlphaBlending(ConfigInfo &config_info);

private:
    static auto ReadFile(const std::string &filepath) -> std::vector<char>;

    void CreateGraphicsPipeline(const std::string &vert_filepath, const std::string &frag_filepath,
                                const ConfigInfo &config_info);

    void CreateShaderModule(const std::vector<char> &code, VkShaderModule *shader_module);

    std::shared_ptr<Device> m_device;
    VkPipeline graphicsPipeline;
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;
};

}// namespace rendering

}// namespace saturn