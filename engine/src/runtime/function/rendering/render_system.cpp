#include "render_system.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace saturn {

namespace rendering {

RenderSystem::RenderSystem(uint32_t width, uint32_t height) : m_width(width), m_height(height) { Init(); }

RenderSystem::~RenderSystem() {
    vkDeviceWaitIdle(m_render_device->GetVkDevice());
    Clear();
}

void RenderSystem::Init() {
    InitWindow();
    InitVulkan();
    InitImgui();
}

void RenderSystem::Tick(float delta_time) {
    UpdateUniformBuffer(m_cur_swapchain_frame_index);


    BeginFrame();

    BeginOffscreenRenderPass();
    {
        m_shadowmap_pipeline->CmdBindCommandBuffer(m_command_builder);

        VkBuffer vertex_buffers[] = {m_render_objects.at(0)->GetVertexBuffer()->GetVkBuffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(m_command_builder->GetCurrentCommandBuffer(), 0, 1, vertex_buffers, offsets);

        vkCmdBindIndexBuffer(m_command_builder->GetCurrentCommandBuffer(),
                             m_render_objects.at(0)->GetIndexBuffer()->GetVkBuffer(), 0, VK_INDEX_TYPE_UINT32);

        m_shadowmap_pipeline->CmdBindDescriptorSets(m_command_builder,
                                                    m_shadowmap_descriptor_sets[m_cur_swapchain_frame_index]);

        vkCmdDrawIndexed(m_command_builder->GetCurrentCommandBuffer(),
                         static_cast<uint32_t>(m_render_objects.at(0)->GetIndices().size()), 1, 0, 0, 0);
    }
    EndOffscreenRenderPass();


    BeginShadingRenderPass();
    {
        m_shading_pipeline->CmdBindCommandBuffer(m_command_builder);

        VkBuffer vertex_buffers[] = {m_render_objects.at(0)->GetVertexBuffer()->GetVkBuffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(m_command_builder->GetCurrentCommandBuffer(), 0, 1, vertex_buffers, offsets);

        vkCmdBindIndexBuffer(m_command_builder->GetCurrentCommandBuffer(),
                             m_render_objects.at(0)->GetIndexBuffer()->GetVkBuffer(), 0, VK_INDEX_TYPE_UINT32);

        //TODO(整理代码)
        VkDescriptorImageInfo shadowmap_image_info{};
        shadowmap_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        shadowmap_image_info.imageView = m_render_swapchain->GetShadowmapImage()->GetVkImageView();
        shadowmap_image_info.sampler = m_texture_sampler;
        rendering::DescriptorWriter(m_descriptor_set_layout, m_descriptor_pool)
                .WriteImage(2, &shadowmap_image_info)
                .Overwrite(m_descriptor_sets.at(m_cur_swapchain_frame_index));

        m_shading_pipeline->CmdBindDescriptorSets(m_command_builder, m_descriptor_sets[m_cur_swapchain_frame_index]);

        vkCmdDrawIndexed(m_command_builder->GetCurrentCommandBuffer(),
                         static_cast<uint32_t>(m_render_objects.at(0)->GetIndices().size()), 1, 0, 0, 0);

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Text("FPS:%i", static_cast<int>(1.0f / delta_time));
        // ImGui::ShowDemoWindow();

        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_command_builder->GetCurrentCommandBuffer());
    }
    EndShadingRenderPass();

    EndFrame();
    ++m_test;
}

void RenderSystem::Clear() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    vkDestroySampler(m_render_device->GetVkDevice(), m_texture_sampler, nullptr);

    glfwTerminate();
}

auto RenderSystem::ShouldCloseWindow() -> bool { return glfwWindowShouldClose(m_window->GetGlfwWindow()) != 0; }

void RenderSystem::InitWindow() { m_window = std::make_shared<rendering::Window>(m_width, m_height, "First Game"); }

void RenderSystem::InitVulkan() {
    CreateDevice();
    CreateSwapchain();
    CreateDescriptorSetLayout();
    CreateShadowmapPipeline();
    CreateGraphicsPipeline();
    CreateImage();
    CreateImageSampler();
    LoadModel();
    CreateUniformBuffers();
    CreateDescriptorPool();
    CreateDescriptorSets();
    CreateCommandBuffers();
}

void RenderSystem::InitImgui() {
    //1: create descriptor pool for IMGUI.
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

    ImGui_ImplVulkan_Init(&init_info, m_render_swapchain->GetShadingRenderPass());

    //execute a gpu command to upload imgui font textures
    rendering::CommandsBuilder cmd_builder{m_render_device};
    cmd_builder.AllocateCommandBuffers(1).BeginRecord();
    ImGui_ImplVulkan_CreateFontsTexture(cmd_builder.GetCurrentCommandBuffer());
    cmd_builder.EndRecord().SubmitTo(m_render_device->GetGraphicsQueue());

    //clear font textures from cpu data
    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void RenderSystem::CreateDevice() {
    m_render_device = std::make_shared<rendering::Device>("SaturnEngine", "First Game", m_window);
}

void RenderSystem::CreateSwapchain() { m_render_swapchain = std::make_unique<rendering::Swapchain>(m_render_device); }

void RenderSystem::CreateDescriptorSetLayout() {
    m_shadowmap_descriptor_set_layout =
            rendering::DescriptorSetLayout::Builder(m_render_device)
                    .AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
                    .Build();

    m_descriptor_set_layout =
            rendering::DescriptorSetLayout::Builder(m_render_device)
                    .AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
                    .AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                    .AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                    .Build();
}

void RenderSystem::CreateShadowmapPipeline() {
    std::string vert_path{R"(\shaders\shadow_map.vert.spv)"};
    std::string frag_path{R"(\shaders\shadow_map.frag.spv)"};

    m_shadowmap_pipeline = rendering::Pipeline::Builder(m_render_device)
                                   .BindShaders(vert_path, frag_path)
                                   .BindDescriptorSetLayout(m_shadowmap_descriptor_set_layout)
                                   .BindRenderpass(m_render_swapchain->GetOffscreenRenderPass())
                                   .Build();
}

void RenderSystem::CreateGraphicsPipeline() {
    std::string vert_path{R"(\shaders\shading.vert.spv)"};
    std::string frag_path{R"(\shaders\shading.frag.spv)"};

    m_shading_pipeline = rendering::Pipeline::Builder(m_render_device)
                                  .BindShaders(vert_path, frag_path)
                                  .BindDescriptorSetLayout(m_descriptor_set_layout)
                                  .BindRenderpass(m_render_swapchain->GetShadingRenderPass())
                                  .SetMsaaSamples(m_render_device->GetMaxMsaaSamples())
                                  .EnableAlphaBlending()
                                  .Build();
}

void RenderSystem::CreateImage() {
    // std::string texture_path{R"(\textures\viking_room.png)"};
    std::string texture_path{R"(\textures\japanese_temple.png)"};
    m_render_image = std::make_shared<rendering::Image>(texture_path, m_render_device, VK_SAMPLE_COUNT_1_BIT,
                                                        VK_FORMAT_R8G8B8A8_SRGB);
    m_render_image->CreateImageView(VK_IMAGE_ASPECT_COLOR_BIT);
}

void RenderSystem::CreateImageSampler() {
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

    if (vkCreateSampler(m_render_device->GetVkDevice(), &sampler_info, nullptr, &m_texture_sampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

void RenderSystem::LoadModel() {
    // std::string model_path{R"(\models\viking_room.obj)"};
    std::string temple_model_path{R"(\models\japanese_temple.obj)"};
    std::string floor_model_path{R"(\models\floor.obj)"};

    m_render_objects.push_back(std::make_shared<rendering::RenderObject>(
            m_render_device, std::make_unique<resource::Model>(ENGINE_ROOT_DIR + temple_model_path)));
    m_render_objects.push_back(std::make_shared<rendering::RenderObject>(
            m_render_device, std::make_unique<resource::Model>(ENGINE_ROOT_DIR + floor_model_path)));
}

void RenderSystem::CreateUniformBuffers() {
    VkDeviceSize buffer_size = sizeof(UniformBufferObject);
    m_uniform_buffers.resize(m_render_swapchain->GetMaxFramesInFlight());

    for (size_t i = 0; i < m_render_swapchain->GetMaxFramesInFlight(); i++) {
        m_uniform_buffers.at(i) = std::make_shared<rendering::Buffer>(
                m_render_device, buffer_size, 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        m_uniform_buffers.at(i)->Map();
    }
}

void RenderSystem::CreateDescriptorPool() {
    m_descriptor_pool = rendering::DescriptorPool::Builder(m_render_device)
                                .AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10)
                                .AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10)
                                .SetPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
                                .Build();
}

void RenderSystem::CreateDescriptorSets() {
    // shadowmap
    {
        std::vector<VkDescriptorSetLayout> layouts(m_render_swapchain->GetMaxFramesInFlight(),
                                                   m_shadowmap_descriptor_set_layout->GetDescriptorSetLayout());
        m_shadowmap_descriptor_sets.resize(m_render_swapchain->GetMaxFramesInFlight());

        for (int i = 0; i < m_render_swapchain->GetMaxFramesInFlight(); ++i) {
            m_descriptor_pool->AllocateDescriptor(layouts.at(i), m_shadowmap_descriptor_sets.at(i));
        }

        for (size_t i = 0; i < m_render_swapchain->GetMaxFramesInFlight(); ++i) {
            VkDescriptorBufferInfo buffer_info{};
            buffer_info.buffer = m_uniform_buffers.at(i)->GetVkBuffer();
            buffer_info.offset = 0;
            buffer_info.range = sizeof(UniformBufferObject);

            rendering::DescriptorWriter(m_shadowmap_descriptor_set_layout, m_descriptor_pool)
                    .WriteBuffer(0, &buffer_info)
                    .Overwrite(m_shadowmap_descriptor_sets.at(i));
        }
    }

    // shading
    {
        std::vector<VkDescriptorSetLayout> layouts(m_render_swapchain->GetMaxFramesInFlight(),
                                                   m_descriptor_set_layout->GetDescriptorSetLayout());
        m_descriptor_sets.resize(m_render_swapchain->GetMaxFramesInFlight());

        for (int i = 0; i < m_render_swapchain->GetMaxFramesInFlight(); ++i) {
            m_descriptor_pool->AllocateDescriptor(layouts.at(i), m_descriptor_sets.at(i));
        }

        for (size_t i = 0; i < m_render_swapchain->GetMaxFramesInFlight(); i++) {
            VkDescriptorBufferInfo buffer_info{};
            buffer_info.buffer = m_uniform_buffers.at(i)->GetVkBuffer();
            buffer_info.offset = 0;
            buffer_info.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo image_info{};
            image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_info.imageView = m_render_image->GetVkImageView();
            image_info.sampler = m_texture_sampler;

            VkDescriptorImageInfo shadowmap_image_info{};
            shadowmap_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            shadowmap_image_info.imageView = m_render_swapchain->GetShadowmapImage()->GetVkImageView();
            shadowmap_image_info.sampler = m_texture_sampler;

            rendering::DescriptorWriter(m_descriptor_set_layout, m_descriptor_pool)
                    .WriteBuffer(0, &buffer_info)
                    .WriteImage(1, &image_info)
                    .WriteImage(2, &shadowmap_image_info)
                    .Overwrite(m_descriptor_sets.at(i));
        }
    }
}

void RenderSystem::CreateCommandBuffers() {
    m_command_builder = std::make_shared<rendering::CommandsBuilder>(m_render_device);
    m_command_builder->AllocateCommandBuffers(m_render_swapchain->GetMaxFramesInFlight());
}

void RenderSystem::RecreateSwapchain() {
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

void RenderSystem::UpdateUniformBuffer(uint32_t current_frame_index) {
    static auto start_time = std::chrono::high_resolution_clock::now();

    auto current_time = std::chrono::high_resolution_clock::now();
    float accumulate_time =
            std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

    // 右手坐标系，z轴指向屏幕外，y轴向上，x轴指向右侧
    UniformBufferObject ubo{};
    auto eye_pos = glm::vec3(2.0f, 1.5f, 2.0f);

    // 非均匀缩放时需要考虑法线的问题
    ubo.model = glm::mat4(1.0f);
    ubo.model = glm::rotate(ubo.model, accumulate_time * glm::radians(20.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.view = glm::lookAt(eye_pos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.proj = glm::perspective(
            glm::radians(45.0f),
            m_render_swapchain->Extent().width / static_cast<float>(m_render_swapchain->Extent().height), 0.1f, 5.0f);

    // auto light_perspective = glm::perspective(
    //         glm::radians(45.0f),
    //         m_render_swapchain->Extent().width / static_cast<float>(m_render_swapchain->Extent().height), 0.1f, 1000.0f);

    auto light_perspective = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 5.0f);
    auto light_pos = glm::vec3(-2.0f, 2.0f, 2.0f);

    ubo.light_view_proj = light_perspective * glm::lookAt(light_pos, glm::vec3(0.0f, 0.0f, 0.0f),
                                                          glm::vec3(0.0f, 1.0f, 0.0f));

    m_uniform_buffers.at(current_frame_index)->WriteToBuffer(&ubo);
}

void RenderSystem::BeginFrame() {
    vkWaitForFences(m_render_device->GetVkDevice(), 1,
                    &m_render_swapchain->GetInFlightFences()[m_cur_swapchain_frame_index], VK_TRUE, UINT64_MAX);
    auto [result, image_index] = m_render_swapchain->AcquireNextImage(m_cur_swapchain_frame_index);
    m_image_index = image_index;

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapchain();
        return;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    m_command_builder->SetCurrentCommandBuffer(m_cur_swapchain_frame_index).BeginRecord();
}

void RenderSystem::EndFrame() {
    m_command_builder->EndRecord();

    vkResetFences(m_render_device->GetVkDevice(), 1,
                  &m_render_swapchain->GetInFlightFences()[m_cur_swapchain_frame_index]);
    // semaphores
    std::vector<VkSemaphore> image_available_semaphores = {
            m_render_swapchain
                    ->GetImageAvailableSemaphores()[m_cur_swapchain_frame_index]};// 等待当前帧缓冲准备好后，进行渲染
    std::vector<VkPipelineStageFlags> wait_stages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    std::vector<VkSemaphore> render_finished_semaphores = {
            m_render_swapchain->GetRenderFinishedSemaphores()[m_cur_swapchain_frame_index]};

    m_command_builder->WaitForSemaphoresAndStages(image_available_semaphores, wait_stages)
            .SignalSemaphores(render_finished_semaphores)
            .SignalFence(m_render_swapchain->GetInFlightFences()[m_cur_swapchain_frame_index])
            .SubmitTo(m_render_device->GetGraphicsQueue());

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = render_finished_semaphores.data();

    VkSwapchainKHR swap_chains[] = {m_render_swapchain->VkSwapchain()};
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swap_chains;

    present_info.pImageIndices = &m_image_index;

    auto result = vkQueuePresentKHR(m_render_device->GetPresentQueue(), &present_info);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_window->m_has_resized) {
        m_window->m_has_resized = false;
        RecreateSwapchain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    m_cur_swapchain_frame_index = (m_cur_swapchain_frame_index + 1) % m_render_swapchain->GetMaxFramesInFlight();
}

void RenderSystem::BeginOffscreenRenderPass() {
    auto *cmd_buffer = m_command_builder->GetCurrentCommandBuffer();

    // 获取shadowmap的尺寸
    auto [shadow_map_width, shadow_map_height] = m_render_swapchain->GetShadowmapImage()->GetExtent();

    VkRenderPassBeginInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = m_render_swapchain->GetOffscreenRenderPass();
    render_pass_info.framebuffer = m_render_swapchain->GetOffscreenFramebuffer()[m_image_index];
    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent = {shadow_map_width, shadow_map_height};

    std::array<VkClearValue, 1> clear_values{};
    clear_values[0].depthStencil = {1.0f, 0};

    render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
    render_pass_info.pClearValues = clear_values.data();

    vkCmdBeginRenderPass(cmd_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    // 基于VK_KHR_Maintenance1扩展，通过设置负的视口来抵消vulkan的NDC坐标y轴向下的问题
    viewport.y = static_cast<float>(shadow_map_height); // 这里一定要强转成float
    viewport.width = static_cast<float>(shadow_map_width);
    viewport.height = -static_cast<float>(shadow_map_height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd_buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {shadow_map_width, shadow_map_height};
    vkCmdSetScissor(cmd_buffer, 0, 1, &scissor);
}

void RenderSystem::EndOffscreenRenderPass() { vkCmdEndRenderPass(m_command_builder->GetCurrentCommandBuffer()); }

void RenderSystem::BeginShadingRenderPass() {
    auto *cmd_buffer = m_command_builder->GetCurrentCommandBuffer();

    VkRenderPassBeginInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = m_render_swapchain->GetShadingRenderPass();
    render_pass_info.framebuffer = m_render_swapchain->GetFramebuffer()[m_image_index];
    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent = m_render_swapchain->Extent();

    std::array<VkClearValue, 2> clear_values{};
    clear_values[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clear_values[1].depthStencil = {1.0f, 0};

    render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
    render_pass_info.pClearValues = clear_values.data();

    vkCmdBeginRenderPass(cmd_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    // 基于VK_KHR_Maintenance1扩展，通过设置负的视口来抵消vulkan的NDC坐标y轴向下的问题
    viewport.y = m_render_swapchain->Extent().height;
    viewport.width = static_cast<float>(m_render_swapchain->Extent().width);
    viewport.height = -static_cast<float>(m_render_swapchain->Extent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd_buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_render_swapchain->Extent();
    vkCmdSetScissor(cmd_buffer, 0, 1, &scissor);
}

void RenderSystem::EndShadingRenderPass() { vkCmdEndRenderPass(m_command_builder->GetCurrentCommandBuffer()); }

}// namespace rendering

}// namespace saturn