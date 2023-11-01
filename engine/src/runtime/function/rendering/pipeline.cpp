// #include "pipeline.hpp"

// #include <runtime/resource/model.hpp>

// namespace saturn {

// namespace rendering {

// Pipeline::ConfigInfo::ConfigInfo() {
//     inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
//     inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
//     inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

//     viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
//     viewportInfo.viewportCount = 1;
//     viewportInfo.pViewports = nullptr;
//     viewportInfo.scissorCount = 1;
//     viewportInfo.pScissors = nullptr;

//     rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
//     rasterizationInfo.depthClampEnable = VK_FALSE;
//     rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
//     rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
//     rasterizationInfo.lineWidth = 1.0f;
//     rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
//     rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
//     rasterizationInfo.depthBiasEnable = VK_FALSE;
//     rasterizationInfo.depthBiasConstantFactor = 0.0f;// Optional
//     rasterizationInfo.depthBiasClamp = 0.0f;         // Optional
//     rasterizationInfo.depthBiasSlopeFactor = 0.0f;   // Optional

//     multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
//     multisampleInfo.sampleShadingEnable = VK_FALSE;
//     multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
//     multisampleInfo.minSampleShading = 1.0f;         // Optional
//     multisampleInfo.pSampleMask = nullptr;           // Optional
//     multisampleInfo.alphaToCoverageEnable = VK_FALSE;// Optional
//     multisampleInfo.alphaToOneEnable = VK_FALSE;     // Optional

//     colorBlendAttachment.colorWriteMask =
//             VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
//     colorBlendAttachment.blendEnable = VK_FALSE;
//     colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
//     colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;// Optional
//     colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;            // Optional
//     colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
//     colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;// Optional
//     colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;            // Optional

//     colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
//     colorBlendInfo.logicOpEnable = VK_FALSE;
//     colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;// Optional
//     colorBlendInfo.attachmentCount = 1;
//     colorBlendInfo.pAttachments = &colorBlendAttachment;
//     colorBlendInfo.blendConstants[0] = 0.0f;// Optional
//     colorBlendInfo.blendConstants[1] = 0.0f;// Optional
//     colorBlendInfo.blendConstants[2] = 0.0f;// Optional
//     colorBlendInfo.blendConstants[3] = 0.0f;// Optional

//     depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
//     depthStencilInfo.depthTestEnable = VK_TRUE;
//     depthStencilInfo.depthWriteEnable = VK_TRUE;
//     depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
//     depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
//     depthStencilInfo.minDepthBounds = 0.0f;// Optional
//     depthStencilInfo.maxDepthBounds = 1.0f;// Optional
//     depthStencilInfo.stencilTestEnable = VK_FALSE;
//     depthStencilInfo.front = {};// Optional
//     depthStencilInfo.back = {}; // Optional

//     dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
//     dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
//     dynamicStateInfo.pDynamicStates = dynamicStateEnables.data();
//     dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
//     dynamicStateInfo.flags = 0;

//     bindingDescriptions = resource::Model::Vertex::GetBindingDescriptions();
//     attributeDescriptions = resource::Model::Vertex::GetAttributeDescriptions();
// }

// Pipeline::Pipeline(std::shared_ptr<Device> device, const std::string &vert_filepath,
//                                const std::string &frag_filepath, const ConfigInfo &config_info)
//     : m_device(std::move(device)) {
//     CreateGraphicsPipeline(vert_filepath, frag_filepath, config_info);
// }

// void Pipeline::CreateGraphicsPipeline(const std::string &vert_filepath, const std::string &frag_filepath,
//                                             const ConfigInfo &config_info) {
//     SATURN_ASSERT(config_info.pipelineLayout != VK_NULL_HANDLE,
//                   "Cannot create graphics pipeline: no pipelineLayout provided in configInfo");
//     SATURN_ASSERT(config_info.renderPass != VK_NULL_HANDLE,
//                   "Cannot create graphics pipeline: no renderPass provided in configInfo");

//     auto vert_code = (vert_filepath);
//     auto frag_code = readFile(frag_filepath);

//     createShaderModule(vertCode, &vertShaderModule);
//     createShaderModule(fragCode, &fragShaderModule);

//     VkPipelineShaderStageCreateInfo shaderStages[2];
//     shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//     shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
//     shaderStages[0].module = vertShaderModule;
//     shaderStages[0].pName = "main";
//     shaderStages[0].flags = 0;
//     shaderStages[0].pNext = nullptr;
//     shaderStages[0].pSpecializationInfo = nullptr;
//     shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//     shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
//     shaderStages[1].module = fragShaderModule;
//     shaderStages[1].pName = "main";
//     shaderStages[1].flags = 0;
//     shaderStages[1].pNext = nullptr;
//     shaderStages[1].pSpecializationInfo = nullptr;

//     const auto &binding_descriptions = configInfo.bindingDescriptions;
//     const auto &attribute_descriptions = configInfo.attributeDescriptions;
//     VkPipelineVertexInputStateCreateInfo vertex_input_info{};
//     vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
//     vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
//     vertex_input_info.vertexBindingDescriptionCount = static_cast<uint32_t>(binding_descriptions.size());
//     vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();
//     vertex_input_info.pVertexBindingDescriptions = binding_descriptions.data();

//     VkGraphicsPipelineCreateInfo pipeline_info{};
//     pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
//     pipeline_info.stageCount = 2;
//     pipeline_info.pStages = shaderStages;
//     pipeline_info.pVertexInputState = &vertex_input_info;
//     pipeline_info.pInputAssemblyState = &configInfo.inputAssemblyInfo;
//     pipeline_info.pViewportState = &configInfo.viewportInfo;
//     pipeline_info.pRasterizationState = &configInfo.rasterizationInfo;
//     pipeline_info.pMultisampleState = &configInfo.multisampleInfo;
//     pipeline_info.pColorBlendState = &configInfo.colorBlendInfo;
//     pipeline_info.pDepthStencilState = &configInfo.depthStencilInfo;
//     pipeline_info.pDynamicState = &configInfo.dynamicStateInfo;

//     pipeline_info.layout = configInfo.pipelineLayout;
//     pipeline_info.renderPass = configInfo.renderPass;
//     pipeline_info.subpass = configInfo.subpass;

//     pipeline_info.basePipelineIndex = -1;
//     pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

//     if (vkCreateGraphicsPipelines(lveDevice.device(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphicsPipeline) !=
//         VK_SUCCESS) {
//         throw std::runtime_error("failed to create graphics pipeline");
//     }
// }

// }

// }// namespace saturn