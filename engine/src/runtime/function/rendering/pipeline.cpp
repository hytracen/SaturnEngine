#include "pipeline.hpp"

#include <runtime/resource/file_helper.hpp>
#include <runtime/resource/model.hpp>
#include <utility>


namespace saturn {

namespace rendering {

Pipeline::ConfigInfo::ConfigInfo() {
    m_input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    m_input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    m_input_assembly_info.primitiveRestartEnable = VK_FALSE;

    m_viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    m_viewport_info.viewportCount = 1;
    m_viewport_info.pViewports = nullptr;
    m_viewport_info.scissorCount = 1;
    m_viewport_info.pScissors = nullptr;

    m_rasterization_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    m_rasterization_info.depthClampEnable = VK_FALSE;
    m_rasterization_info.rasterizerDiscardEnable = VK_FALSE;
    m_rasterization_info.polygonMode = VK_POLYGON_MODE_FILL;
    m_rasterization_info.lineWidth = 1.0f;
    m_rasterization_info.cullMode = VK_CULL_MODE_NONE;
    m_rasterization_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    m_rasterization_info.depthBiasEnable = VK_FALSE;
    m_rasterization_info.depthBiasConstantFactor = 0.0f;// Optional
    m_rasterization_info.depthBiasClamp = 0.0f;         // Optional
    m_rasterization_info.depthBiasSlopeFactor = 0.0f;   // Optional

    m_multisample_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    m_multisample_info.sampleShadingEnable = VK_FALSE;
    m_multisample_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    m_multisample_info.minSampleShading = 1.0f;         // Optional
    m_multisample_info.pSampleMask = nullptr;           // Optional
    m_multisample_info.alphaToCoverageEnable = VK_FALSE;// Optional
    m_multisample_info.alphaToOneEnable = VK_FALSE;     // Optional

    m_color_blend_attachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    m_color_blend_attachment.blendEnable = VK_FALSE;
    m_color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    m_color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;// Optional
    m_color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;            // Optional
    m_color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    m_color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;// Optional
    m_color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;            // Optional

    m_color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    m_color_blend_info.logicOpEnable = VK_FALSE;
    m_color_blend_info.logicOp = VK_LOGIC_OP_COPY;// Optional
    m_color_blend_info.attachmentCount = 1;
    m_color_blend_info.pAttachments = &m_color_blend_attachment;
    m_color_blend_info.blendConstants[0] = 0.0f;// Optional
    m_color_blend_info.blendConstants[1] = 0.0f;// Optional
    m_color_blend_info.blendConstants[2] = 0.0f;// Optional
    m_color_blend_info.blendConstants[3] = 0.0f;// Optional

    m_depth_stencil_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    m_depth_stencil_info.depthTestEnable = VK_TRUE;
    m_depth_stencil_info.depthWriteEnable = VK_TRUE;
    m_depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS;
    m_depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
    m_depth_stencil_info.minDepthBounds = 0.0f;// Optional
    m_depth_stencil_info.maxDepthBounds = 1.0f;// Optional
    m_depth_stencil_info.stencilTestEnable = VK_FALSE;
    m_depth_stencil_info.front = {};// Optional
    m_depth_stencil_info.back = {}; // Optional

    m_dynamic_state_enables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    m_dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    m_dynamic_state_info.pDynamicStates = m_dynamic_state_enables.data();
    m_dynamic_state_info.dynamicStateCount = static_cast<uint32_t>(m_dynamic_state_enables.size());
    m_dynamic_state_info.flags = 0;
    m_dynamic_state_info.pNext = nullptr;

    m_binding_descriptions = resource::Model::Vertex::GetBindingDescriptions();
    m_attribute_descriptions = resource::Model::Vertex::GetAttributeDescriptions();
}

Pipeline::Builder::Builder(std::shared_ptr<Device> device) : m_device(std::move(device)) {
    m_config_info = std::make_shared<ConfigInfo>();
}

auto Pipeline::Builder::EnableAlphaBlending() -> Builder & {
    m_config_info->m_color_blend_attachment.blendEnable = VK_TRUE;
    m_config_info->m_color_blend_attachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    m_config_info->m_color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    m_config_info->m_color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    m_config_info->m_color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    m_config_info->m_color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    m_config_info->m_color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    m_config_info->m_color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    return *this;
}

auto Pipeline::Builder::BindShaders(std::string vert_shader_path, std::string frag_shader_path) -> Builder & {
    m_vert_path = std::move(vert_shader_path);
    m_frag_path = std::move(frag_shader_path);
    return *this;
}

auto Pipeline::Builder::BindDescriptorSetLayout(std::shared_ptr<DescriptorSetLayout> descriptor_set_layout)
        -> Builder & {
    std::vector<VkDescriptorSetLayout> descriptor_sets_layouts{descriptor_set_layout->GetDescriptorSetLayout()};
    VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount = descriptor_sets_layouts.size();
    pipeline_layout_create_info.pSetLayouts = descriptor_sets_layouts.data();
    // pipeline_layout_info.pushConstantRangeCount = 1;
    // pipeline_layout_info.pPushConstantRanges = &push_constant_range;
    if (vkCreatePipelineLayout(m_device->GetVkDevice(), &pipeline_layout_create_info, nullptr,
                               &m_config_info->m_pipeline_layout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    return *this;
}

auto Pipeline::Builder::BindRenderpass(VkRenderPass render_pass) -> Builder & {
    m_config_info->m_render_pass = render_pass;
    return *this;
}

auto Pipeline::Builder::SetMsaaSamples(VkSampleCountFlagBits sample_count) -> Builder & {
    m_config_info->m_multisample_info.rasterizationSamples = sample_count;
    return *this;
}

auto Pipeline::Builder::Build() -> std::shared_ptr<Pipeline> {
    return std::make_shared<Pipeline>(m_device, m_vert_path, m_frag_path, m_config_info);
}

Pipeline::Pipeline(std::shared_ptr<Device> device, const std::string &vert_filepath, const std::string &frag_filepath,
                   std::shared_ptr<ConfigInfo> config_info)
    : m_device(std::move(device)), m_config_info(std::move(config_info)) {
    CreateGraphicsPipeline(vert_filepath, frag_filepath, m_config_info);
}

Pipeline::~Pipeline() {
    vkDestroyPipeline(m_device->GetVkDevice(), m_graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(m_device->GetVkDevice(), m_config_info->m_pipeline_layout, nullptr);
}

void Pipeline::CmdBindCommandBuffer(std::shared_ptr<CommandsBuilder> cmd_builder) {
    vkCmdBindPipeline(cmd_builder->GetCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphics_pipeline);
}

void Pipeline::CmdBindDescriptorSets(std::shared_ptr<CommandsBuilder> cmd_builder,
                                     VkDescriptorSet descriptor_set) const {
    vkCmdBindDescriptorSets(cmd_builder->GetCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_config_info->m_pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);
}

void Pipeline::CreateGraphicsPipeline(const std::string &vert_filepath, const std::string &frag_filepath,
                                      std::shared_ptr<ConfigInfo> config_info) {
    SATURN_ASSERT(config_info->m_pipeline_layout != VK_NULL_HANDLE,
                  "Cannot create graphics pipeline: no pipelineLayout provided in configInfo");
    SATURN_ASSERT(config_info->m_render_pass != VK_NULL_HANDLE,
                  "Cannot create graphics pipeline: no renderPass provided in configInfo");

    auto vert_code = resource::FileHelper::ReadFile(vert_filepath);
    auto frag_code = resource::FileHelper::ReadFile(frag_filepath);

    CreateShaderModule(vert_code, &m_vert_shader_module);
    CreateShaderModule(frag_code, &m_frag_shader_module);

    VkPipelineShaderStageCreateInfo shader_stages[2];
    shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stages[0].module = m_vert_shader_module;
    shader_stages[0].pName = "main";
    shader_stages[0].flags = 0;
    shader_stages[0].pNext = nullptr;
    shader_stages[0].pSpecializationInfo = nullptr;
    shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stages[1].module = m_frag_shader_module;
    shader_stages[1].pName = "main";
    shader_stages[1].flags = 0;
    shader_stages[1].pNext = nullptr;
    shader_stages[1].pSpecializationInfo = nullptr;

    const auto &binding_descriptions = config_info->m_binding_descriptions;
    const auto &attribute_descriptions = config_info->m_attribute_descriptions;
    VkPipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
    vertex_input_info.vertexBindingDescriptionCount = static_cast<uint32_t>(binding_descriptions.size());
    vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();
    vertex_input_info.pVertexBindingDescriptions = binding_descriptions.data();

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &config_info->m_input_assembly_info;
    pipeline_info.pViewportState = &config_info->m_viewport_info;
    pipeline_info.pRasterizationState = &config_info->m_rasterization_info;
    pipeline_info.pMultisampleState = &config_info->m_multisample_info;
    pipeline_info.pColorBlendState = &config_info->m_color_blend_info;
    pipeline_info.pDepthStencilState = &config_info->m_depth_stencil_info;
    pipeline_info.pDynamicState = &config_info->m_dynamic_state_info;

    pipeline_info.layout = config_info->m_pipeline_layout;
    pipeline_info.renderPass = config_info->m_render_pass;
    pipeline_info.subpass = config_info->m_subpass;

    pipeline_info.basePipelineIndex = -1;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(m_device->GetVkDevice(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr,
                                  &m_graphics_pipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline");
    }

    vkDestroyShaderModule(m_device->GetVkDevice(), m_frag_shader_module, nullptr);
    vkDestroyShaderModule(m_device->GetVkDevice(), m_vert_shader_module, nullptr);
}

void Pipeline::CreateShaderModule(const std::vector<char> &code, VkShaderModule *shader_module) {
    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32_t *>(code.data());

    if (vkCreateShaderModule(m_device->GetVkDevice(), &create_info, nullptr, shader_module) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module");
    }
}

}// namespace rendering

}// namespace saturn