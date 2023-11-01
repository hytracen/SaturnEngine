#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <engine_pch.hpp>
#include <runtime/function/rendering/buffer.hpp>
#include <runtime/function/rendering/commands.hpp>
#include <runtime/function/rendering/descriptor.hpp>
#include <runtime/function/rendering/device.hpp>
#include <runtime/function/rendering/image.hpp>
#include <runtime/function/rendering/swapchain.hpp>
#include <runtime/function/rendering/window.hpp>
#include <runtime/resource/model.hpp>

namespace saturn {

const uint32_t kWidth = 800;
const uint32_t kHeight = 600;

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

class HelloTriangleApplication {
public:
    void Run() {
        InitWindow();
        InitVulkan();
        InitImgui();
        MainLoop();
        CleanUp();
    }

private:
    // GLFWwindow *window;
    std::shared_ptr<rendering::Window> m_window;
    std::shared_ptr<rendering::Device> m_render_device;
    std::unique_ptr<rendering::Swapchain> m_render_swapchain;

    // VkDescriptorSetLayout descriptorSetLayout;
    std::shared_ptr<rendering::DescriptorPool> m_descriptor_pool;
    std::shared_ptr<rendering::DescriptorPool> m_imgui_descriptor_pool;
    std::shared_ptr<rendering::DescriptorSetLayout> m_descriptor_set_layout;

    std::shared_ptr<rendering::Image> m_render_image;

    std::vector<VkDescriptorSet> m_descriptor_sets;

    std::shared_ptr<rendering::Buffer> m_vertex_buffer;
    std::shared_ptr<rendering::Buffer> m_index_buffer;

    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    uint32_t mipLevels;
    VkImageView textureImageView;
    VkSampler textureSampler;

    std::vector<std::shared_ptr<resource::Model>> m_models;

    std::vector<std::shared_ptr<rendering::Buffer>> m_uniform_buffers;

    std::shared_ptr<rendering::CommandsBuilder> m_command_builder;


    uint32_t currentFrame = 0;

    void InitWindow() { m_window = std::make_shared<rendering::Window>(kWidth, kHeight, "First Game"); }

    void InitVulkan() {
        m_render_device = std::make_shared<rendering::Device>("SaturnEngine", "First Game", m_window);
        m_render_swapchain = std::make_unique<rendering::Swapchain>(m_render_device);

        CreateDescriptorSetLayout();
        CreateGraphicsPipeline();
        CreateTextureImage();
        CreateTextureImageView();
        CreateTextureSampler();
        LoadModel();
        CreateVertexBuffer();
        CreateIndexBuffer();
        CreateUniformBuffers();
        CreateDescriptorPool();
        CreateDescriptorSets();
        CreateCommandBuffers();
    }

    void InitImgui() {
        //1: create descriptor pool for IMGUI
        // the size of the pool is very oversize, but it's copied from imgui demo itself.
        m_imgui_descriptor_pool = rendering::DescriptorPool::Builder(m_render_device)
                                          .AddPoolSize(VK_DESCRIPTOR_TYPE_SAMPLER, 1000)
                                          .AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000)
                                          .AddPoolSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000)
                                          .AddPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000)
                                          .AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000)
                                          .AddPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000)
                                          .AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000)
                                          .AddPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000)
                                          .AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000)
                                          .AddPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000)
                                          .AddPoolSize(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000)
                                          .SetPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
                                          .Build();

        // 2: initialize imgui library

        //this initializes the core structures of imgui
        ImGui::CreateContext();

        //this initializes imgui for SDL
        ImGui_ImplGlfw_InitForVulkan(m_window->GetGlfwWindow(), true);

        //this initializes imgui for Vulkan
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = m_render_device->GetVkInstance();
        init_info.PhysicalDevice = m_render_device->GetPhyDevice();
        init_info.Device = m_render_device->GetVkDevice();
        init_info.Queue = m_render_device->GetGraphicsQueue();
        init_info.DescriptorPool = m_imgui_descriptor_pool->GetDescriptorPool();
        init_info.MinImageCount = 3;
        init_info.ImageCount = 3;
        init_info.MSAASamples = m_render_device->GetMaxMsaaSamples();

        ImGui_ImplVulkan_Init(&init_info, m_render_swapchain->GetRenderPass());

        //execute a gpu command to upload imgui font textures
        rendering::CommandsBuilder cmd_builder{m_render_device};
        cmd_builder.AllocateCommandBuffers(1).BeginRecord();
        // auto *cmd = BeginSingleTimeCommands();
        ImGui_ImplVulkan_CreateFontsTexture(cmd_builder.GetCurrentCommandBuffer());
        cmd_builder.EndRecord().SubmitTo(m_render_device->GetGraphicsQueue());

        //clear font textures from cpu data
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    void MainLoop() {
        while (!glfwWindowShouldClose(m_window->GetGlfwWindow())) {
            glfwPollEvents();
            DrawFrame();
        }

        vkDeviceWaitIdle(m_render_device->GetVkDevice());
    }

    void CleanUp() {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        vkDestroyPipeline(m_render_device->GetVkDevice(), graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(m_render_device->GetVkDevice(), pipelineLayout, nullptr);

        vkDestroySampler(m_render_device->GetVkDevice(), textureSampler, nullptr);
        vkDestroyImageView(m_render_device->GetVkDevice(), textureImageView, nullptr);

        glfwTerminate();
    }

    //TODO(Thy): 整合到Renderer中
    void RecreateSwapChain() {
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(m_window->GetGlfwWindow(), &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(m_window->GetGlfwWindow(), &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(m_render_device->GetVkDevice());

        std::shared_ptr<rendering::Swapchain> old_render_swapchain = std::move(m_render_swapchain);
        m_render_swapchain = std::make_unique<rendering::Swapchain>(m_render_device, old_render_swapchain);
        old_render_swapchain.reset();
    }

    void CreateDescriptorSetLayout() {
        m_descriptor_set_layout =
                rendering::DescriptorSetLayout::Builder(m_render_device)
                        .AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
                        .AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                        .Build();
    }

    void CreateGraphicsPipeline() {
        std::string vert_path{R"(\shaders\vert.vert.spv)"};
        std::string frag_path{R"(\shaders\frag.frag.spv)"};
        ENGINE_LOG_INFO("Load vert shader: {}", ENGINE_ROOT_DIR + vert_path);
        ENGINE_LOG_INFO("Load frag shader: {}", ENGINE_ROOT_DIR + frag_path);
        auto vert_shader_code = ReadFile(ENGINE_ROOT_DIR + vert_path);
        auto frag_shader_code = ReadFile(ENGINE_ROOT_DIR + frag_path);

        VkShaderModule vert_shader_module = CreateShaderModule(vert_shader_code);
        VkShaderModule frag_shader_module = CreateShaderModule(frag_shader_code);

        VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
        vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vert_shader_stage_info.module = vert_shader_module;
        vert_shader_stage_info.pName = "main";

        VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
        frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        frag_shader_stage_info.module = frag_shader_module;
        frag_shader_stage_info.pName = "main";

        VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};

        VkPipelineVertexInputStateCreateInfo vertex_input_info{};
        vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto binding_description = resource::Model::Vertex::GetBindingDescriptions();
        auto attribute_descriptions = resource::Model::Vertex::GetAttributeDescriptions();

        vertex_input_info.vertexBindingDescriptionCount = 1;
        vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
        vertex_input_info.pVertexBindingDescriptions = binding_description.data();
        vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

        VkPipelineInputAssemblyStateCreateInfo input_assembly{};
        input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        input_assembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewport_state{};
        viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state.viewportCount = 1;
        viewport_state.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = m_render_device->GetMaxMsaaSamples();

        VkPipelineDepthStencilStateCreateInfo depth_stencil{};
        depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_stencil.depthTestEnable = VK_TRUE;
        depth_stencil.depthWriteEnable = VK_TRUE;
        depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depth_stencil.depthBoundsTestEnable = VK_FALSE;
        depth_stencil.stencilTestEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState color_blend_attachment{};
        color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        color_blend_attachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo color_blending{};
        color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blending.logicOpEnable = VK_FALSE;
        color_blending.logicOp = VK_LOGIC_OP_COPY;
        color_blending.attachmentCount = 1;
        color_blending.pAttachments = &color_blend_attachment;
        color_blending.blendConstants[0] = 0.0f;
        color_blending.blendConstants[1] = 0.0f;
        color_blending.blendConstants[2] = 0.0f;
        color_blending.blendConstants[3] = 0.0f;

        std::vector<VkDynamicState> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic_state{};
        dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
        dynamic_state.pDynamicStates = dynamic_states.data();

        std::vector<VkDescriptorSetLayout> descriptor_sets_layouts{m_descriptor_set_layout->GetDescriptorSetLayout()};

        VkPipelineLayoutCreateInfo pipeline_layout_info{};
        pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_info.setLayoutCount = 1;//TODO(Thy):为什么只有一个
        pipeline_layout_info.pSetLayouts = descriptor_sets_layouts.data();

        if (vkCreatePipelineLayout(m_render_device->GetVkDevice(), &pipeline_layout_info, nullptr, &pipelineLayout) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipeline_info{};
        pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.stageCount = 2;
        pipeline_info.pStages = shader_stages;
        pipeline_info.pVertexInputState = &vertex_input_info;
        pipeline_info.pInputAssemblyState = &input_assembly;
        pipeline_info.pViewportState = &viewport_state;
        pipeline_info.pRasterizationState = &rasterizer;
        pipeline_info.pMultisampleState = &multisampling;
        pipeline_info.pDepthStencilState = &depth_stencil;
        pipeline_info.pColorBlendState = &color_blending;
        pipeline_info.pDynamicState = &dynamic_state;
        pipeline_info.layout = pipelineLayout;
        pipeline_info.renderPass = m_render_swapchain->GetRenderPass();
        pipeline_info.subpass = 0;
        pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(m_render_device->GetVkDevice(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr,
                                      &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(m_render_device->GetVkDevice(), frag_shader_module, nullptr);
        vkDestroyShaderModule(m_render_device->GetVkDevice(), vert_shader_module, nullptr);
    }

    auto HasStencilComponent(VkFormat format) -> bool {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    void CreateTextureImage() {
        int tex_width;
        int tex_height;
        int tex_channels;
        std::string texture_path{R"(\textures\viking_room.png)"};

        stbi_uc *pixels = stbi_load((ENGINE_ROOT_DIR + texture_path).c_str(), &tex_width, &tex_height, &tex_channels,
                                    STBI_rgb_alpha);
        // VkDeviceSize image_size = tex_width * tex_height * 4;
        mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(tex_width, tex_height)))) + 1;

        if (!pixels) { throw std::runtime_error("failed to load texture image!"); }

        rendering::Buffer staging_buffer{m_render_device, 4, static_cast<uint32_t>(tex_width * tex_height),
                                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
        staging_buffer.Map();
        staging_buffer.WriteToBuffer(pixels);
        staging_buffer.Unmap();

        stbi_image_free(pixels);

        m_render_image = std::make_shared<rendering::Image>(
                m_render_device, tex_width, tex_height, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        m_render_image->TransitionToLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        staging_buffer.CopyToImage(m_render_image, static_cast<uint32_t>(tex_width), static_cast<uint32_t>(tex_height));

        GenerateMipmaps(m_render_image->GetVkImage(), VK_FORMAT_R8G8B8A8_SRGB, tex_width, tex_height, mipLevels);
    }

    void GenerateMipmaps(VkImage image, VkFormat image_format, int32_t tex_width, int32_t tex_height,
                         uint32_t mip_levels) {
        // Check if image format supports linear blitting
        VkFormatProperties format_properties;
        vkGetPhysicalDeviceFormatProperties(m_render_device->GetPhyDevice(), image_format, &format_properties);

        if (!(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            throw std::runtime_error("texture image format does not support linear blitting!");
        }

        // VkCommandBuffer command_buffer = BeginSingleTimeCommands();
        rendering::CommandsBuilder cmd_builder{m_render_device};
        cmd_builder.AllocateCommandBuffers(1).BeginRecord();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mip_width = tex_width;
        int32_t mip_height = tex_height;

        for (uint32_t i = 1; i < mip_levels; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(cmd_builder.GetCurrentCommandBuffer(), VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

            VkImageBlit blit{};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {mip_width, mip_height, 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = {mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1};
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(cmd_builder.GetCurrentCommandBuffer(), image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(cmd_builder.GetCurrentCommandBuffer(), VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

            if (mip_width > 1) mip_width /= 2;
            if (mip_height > 1) mip_height /= 2;
        }

        barrier.subresourceRange.baseMipLevel = mip_levels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd_builder.GetCurrentCommandBuffer(), VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        cmd_builder.EndRecord().SubmitTo(m_render_device->GetGraphicsQueue());
    }

    void CreateTextureImageView() {
        textureImageView = CreateImageView(m_render_image->GetVkImage(), VK_FORMAT_R8G8B8A8_SRGB,
                                           VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
    }

    void CreateTextureSampler() {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(m_render_device->GetPhyDevice(), &properties);

        VkSamplerCreateInfo sampler_info{};
        sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_info.magFilter = VK_FILTER_LINEAR;
        sampler_info.minFilter = VK_FILTER_LINEAR;
        sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.anisotropyEnable = VK_TRUE;
        sampler_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        sampler_info.unnormalizedCoordinates = VK_FALSE;
        sampler_info.compareEnable = VK_FALSE;
        sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
        sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler_info.minLod = 0.0f;
        sampler_info.maxLod = VK_LOD_CLAMP_NONE;
        sampler_info.mipLodBias = 0.0f;

        if (vkCreateSampler(m_render_device->GetVkDevice(), &sampler_info, nullptr, &textureSampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    auto CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels)
            -> VkImageView {
        VkImageViewCreateInfo view_info{};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = image;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = format;
        view_info.subresourceRange.aspectMask = aspect_flags;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = mip_levels;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;

        VkImageView image_view;
        if (vkCreateImageView(m_render_device->GetVkDevice(), &view_info, nullptr, &image_view) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image view!");
        }

        return image_view;
    }

    void LoadModel() {
        std::string model_path{R"(\models\viking_room.obj)"};

        m_models.push_back(std::make_shared<resource::Model>(ENGINE_ROOT_DIR + model_path));
        // tinyobj::attrib_t attrib;
        // std::vector<tinyobj::shape_t> shapes;
        // std::vector<tinyobj::material_t> materials;
        // std::string warn;
        // std::string err;

        // std::string model_path{R"(\models\viking_room.obj)"};

        // if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, (ENGINE_ROOT_DIR + model_path).c_str())) {
        //     throw std::runtime_error(warn + err);
        // }

        // std::unordered_map<Vertex, uint32_t> unique_vertices{};

        // for (const auto &shape: shapes) {
        //     for (const auto &index: shape.mesh.indices) {
        //         Vertex vertex{};

        //         vertex.pos = {attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1],
        //                       attrib.vertices[3 * index.vertex_index + 2]};

        //         vertex.texCoord = {attrib.texcoords[2 * index.texcoord_index + 0],
        //                            1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

        //         vertex.color = {1.0f, 1.0f, 1.0f};

        //         if (!unique_vertices.contains(vertex)) {
        //             unique_vertices[vertex] = static_cast<uint32_t>(vertices.size());
        //             vertices.push_back(vertex);
        //         }

        //         indices.push_back(unique_vertices[vertex]);
        //     }
        // }
    }

    void CreateVertexBuffer() {
        auto vertices = m_models.at(0)->m_vertices;

        rendering::Buffer staging_buffer{m_render_device, sizeof(vertices[0]),
                                            static_cast<uint32_t>(vertices.size()), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
        staging_buffer.Map();
        staging_buffer.WriteToBuffer(vertices.data());
        staging_buffer.Unmap();

        m_vertex_buffer = std::make_shared<rendering::Buffer>(
                m_render_device, sizeof(vertices[0]), static_cast<uint32_t>(vertices.size()),
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        staging_buffer.CopyToBuffer(m_vertex_buffer);
    }

    void CreateIndexBuffer() {
        auto indices = m_models.at(0)->m_indices;

        rendering::Buffer staging_buffer{m_render_device, sizeof(indices[0]), static_cast<uint32_t>(indices.size()),
                                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
        staging_buffer.Map();
        staging_buffer.WriteToBuffer(indices.data());
        staging_buffer.Unmap();

        m_index_buffer = std::make_shared<rendering::Buffer>(
                m_render_device, sizeof(indices[0]), static_cast<uint32_t>(indices.size()),
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        staging_buffer.CopyToBuffer(m_index_buffer);
    }

    void CreateUniformBuffers() {
        VkDeviceSize buffer_size = sizeof(UniformBufferObject);
        m_uniform_buffers.resize(m_render_swapchain->m_max_frames_inflight);

        for (size_t i = 0; i < m_render_swapchain->m_max_frames_inflight; i++) {
            m_uniform_buffers.at(i) = std::make_shared<rendering::Buffer>(
                    m_render_device, buffer_size, 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            m_uniform_buffers.at(i)->Map();
        }
    }

    void CreateDescriptorPool() {
        m_descriptor_pool =
                rendering::DescriptorPool::Builder(m_render_device)
                        .AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_render_swapchain->m_max_frames_inflight)
                        .AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                     m_render_swapchain->m_max_frames_inflight)
                        .SetPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
                        .Build();
    }

    void CreateDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(m_render_swapchain->m_max_frames_inflight,
                                                   m_descriptor_set_layout->GetDescriptorSetLayout());
        m_descriptor_sets.resize(m_render_swapchain->m_max_frames_inflight);

        for (int i = 0; i < m_render_swapchain->m_max_frames_inflight; ++i) {
            m_descriptor_pool->AllocateDescriptor(layouts.at(i), m_descriptor_sets.at(i));
        }

        for (size_t i = 0; i < m_render_swapchain->m_max_frames_inflight; i++) {
            VkDescriptorBufferInfo buffer_info{};
            buffer_info.buffer = m_uniform_buffers.at(i)->GetVkBuffer();
            buffer_info.offset = 0;
            buffer_info.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo image_info{};
            image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_info.imageView = textureImageView;
            image_info.sampler = textureSampler;

            rendering::DescriptorWriter render_descriptor_writer{m_descriptor_set_layout, m_descriptor_pool};
            render_descriptor_writer.WriteBuffer(0, &buffer_info)
                    .WriteImage(1, &image_info)
                    .Overwrite(m_descriptor_sets.at(i));
        }
    }

    void CreateCommandBuffers() {
        m_command_builder = std::make_shared<rendering::CommandsBuilder>(m_render_device);
        m_command_builder->AllocateCommandBuffers(2);
    }

    void RecordCommandBuffer(uint32_t image_index) {
        m_command_builder->SetCurrentCommandBuffer(currentFrame).BeginRecord();
        auto *cmd_buffer = m_command_builder->GetCurrentCommandBuffer();

        VkRenderPassBeginInfo render_pass_info{};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass = m_render_swapchain->GetRenderPass();
        render_pass_info.framebuffer = m_render_swapchain->GetFramebuffer()[image_index];
        render_pass_info.renderArea.offset = {0, 0};
        render_pass_info.renderArea.extent = m_render_swapchain->Extent();

        std::array<VkClearValue, 2> clear_values{};
        clear_values[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clear_values[1].depthStencil = {1.0f, 0};

        render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
        render_pass_info.pClearValues = clear_values.data();

        vkCmdBeginRenderPass(cmd_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_render_swapchain->Extent().width);
        viewport.height = static_cast<float>(m_render_swapchain->Extent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd_buffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = m_render_swapchain->Extent();
        vkCmdSetScissor(cmd_buffer, 0, 1, &scissor);

        VkBuffer vertex_buffers[] = {m_vertex_buffer->GetVkBuffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd_buffer, 0, 1, vertex_buffers, offsets);

        vkCmdBindIndexBuffer(cmd_buffer, m_index_buffer->GetVkBuffer(), 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                                &m_descriptor_sets[currentFrame], 0, nullptr);

        vkCmdDrawIndexed(cmd_buffer, static_cast<uint32_t>(m_models.at(0)->m_indices.size()), 1, 0, 0, 0);

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();

        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd_buffer);

        vkCmdEndRenderPass(cmd_buffer);

        m_command_builder->EndRecord();
    }

    void UpdateUniformBuffer(uint32_t current_image) {
        static auto start_time = std::chrono::high_resolution_clock::now();

        auto current_time = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f),
                                    m_render_swapchain->Extent().width /
                                            static_cast<float>(m_render_swapchain->Extent().height),
                                    0.1f, 10.0f);
        ubo.proj[1][1] *= -1;

        m_uniform_buffers.at(current_image)->WriteToBuffer(&ubo);
    }

    void DrawFrame() {
        vkWaitForFences(m_render_device->GetVkDevice(), 1, &m_render_swapchain->InFlightFences()[currentFrame], VK_TRUE,
                        UINT64_MAX);

        uint32_t image_index;
        VkResult result = vkAcquireNextImageKHR(
                m_render_device->GetVkDevice(), m_render_swapchain->VkSwapchain(), UINT64_MAX,
                m_render_swapchain->ImageAvailableSemaphores()[currentFrame], VK_NULL_HANDLE, &image_index);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            RecreateSwapChain();
            return;
        }
        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        UpdateUniformBuffer(currentFrame);

        RecordCommandBuffer(image_index);

        vkResetFences(m_render_device->GetVkDevice(), 1, &m_render_swapchain->InFlightFences()[currentFrame]);

        // vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);

        // semaphores
        std::vector<VkSemaphore> wait_semaphores = {
                m_render_swapchain->ImageAvailableSemaphores()[currentFrame]};// 等待当前帧缓冲准备好后，进行渲染
        std::vector<VkPipelineStageFlags> wait_stages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        std::vector<VkSemaphore> signal_semaphores = {m_render_swapchain->RenderFinishedSemaphores()[currentFrame]};

        m_command_builder->WaitForSemaphoresAndStages(wait_semaphores, wait_stages)
                .SignalSemaphores(signal_semaphores)
                .SignalFence(m_render_swapchain->InFlightFences()[currentFrame])
                .SubmitTo(m_render_device->GetGraphicsQueue());

        VkPresentInfoKHR present_info{};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = signal_semaphores.data();

        VkSwapchainKHR swap_chains[] = {m_render_swapchain->VkSwapchain()};
        present_info.swapchainCount = 1;
        present_info.pSwapchains = swap_chains;

        present_info.pImageIndices = &image_index;

        result = vkQueuePresentKHR(m_render_device->GetPresentQueue(), &present_info);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_window->m_has_resized) {
            m_window->m_has_resized = false;
            RecreateSwapChain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        currentFrame = (currentFrame + 1) % m_render_swapchain->m_max_frames_inflight;
    }

    auto CreateShaderModule(const std::vector<char> &code) -> VkShaderModule {
        VkShaderModuleCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = code.size();
        create_info.pCode = reinterpret_cast<const uint32_t *>(code.data());

        VkShaderModule shader_module;
        if (vkCreateShaderModule(m_render_device->GetVkDevice(), &create_info, nullptr, &shader_module) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }

        return shader_module;
    }

    static auto ReadFile(const std::string &filename) -> std::vector<char> {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) { throw std::runtime_error("failed to open file!"); }

        size_t file_size = static_cast<VkDeviceSize>(file.tellg());
        std::vector<char> buffer(file_size);

        file.seekg(0);
        file.read(buffer.data(), file_size);

        file.close();

        return buffer;
    }
};

}

auto main() -> int {
    saturn::HelloTriangleApplication app;

    try {
        app.Run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
