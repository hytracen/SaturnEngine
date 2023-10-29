#pragma once

#include <engine_pch.hpp>
#include <utility>

#include "render_device.hpp"

namespace saturn {

class RenderDescriptorPool {
public:
    //-----------------------------------Builder-------------------------------------
    class Builder {
    public:
        explicit Builder(std::shared_ptr<RenderDevice> render_device) : m_render_device{std::move(render_device)} {}

        auto AddPoolSize(VkDescriptorType descriptor_type, uint32_t count) -> Builder &;
        auto SetPoolFlags(VkDescriptorPoolCreateFlags flags) -> Builder &;
        // auto SetMaxSets(uint32_t count) -> Builder &;

        [[nodiscard]] auto Build() const -> std::unique_ptr<RenderDescriptorPool>;

    private:
        std::shared_ptr<RenderDevice> m_render_device;
        std::vector<VkDescriptorPoolSize> m_pool_sizes{};
        // uint32_t m_max_sets = 1000;
        uint32_t m_current_sets_count = 0;
        VkDescriptorPoolCreateFlags m_pool_flags = 0;
    };
    //-------------------------------------------------------------------------------


    RenderDescriptorPool(
            std::shared_ptr<RenderDevice> render_device,
            uint32_t max_sets,
            VkDescriptorPoolCreateFlags pool_flags,
            const std::vector<VkDescriptorPoolSize> &pool_sizes);
    ~RenderDescriptorPool();
    RenderDescriptorPool(const RenderDescriptorPool &) = delete;
    auto operator=(const RenderDescriptorPool &) -> RenderDescriptorPool & = delete;


    [[nodiscard]] auto GetDescriptorPool() const -> VkDescriptorPool { return m_descriptor_pool; };

    auto AllocateDescriptor(
            VkDescriptorSetLayout descriptor_set_layout, VkDescriptorSet &descriptor_set) const -> bool;

    void FreeDescriptors(std::vector<VkDescriptorSet> &descriptor_sets) const;

    void ResetPool();

private:
    std::shared_ptr<RenderDevice> m_render_device;
    VkDescriptorPool m_descriptor_pool;

    friend class RenderDescriptorWriter;
};

class RenderDescriptorSetLayout {
public:
    //-----------------------------------Builder-------------------------------------
    class Builder {
    public:
        explicit Builder(std::shared_ptr<RenderDevice> render_device) : m_render_device{std::move(render_device)} {}

        auto AddBinding(uint32_t binding, VkDescriptorType descriptor_type, VkShaderStageFlags stage_flags, uint32_t count = 1) -> Builder &;
        [[nodiscard]] auto Build() const -> std::unique_ptr<RenderDescriptorSetLayout>;

    private:
        std::shared_ptr<RenderDevice> m_render_device;
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
    };
    //-------------------------------------------------------------------------------

    RenderDescriptorSetLayout(std::shared_ptr<RenderDevice> render_device, const std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> &bindings);
    ~RenderDescriptorSetLayout();
    RenderDescriptorSetLayout(const RenderDescriptorSetLayout &) = delete;
    auto operator=(const RenderDescriptorSetLayout &) -> RenderDescriptorSetLayout & = delete;

    [[nodiscard]] auto GetDescriptorSetLayout() const -> VkDescriptorSetLayout { return m_descriptor_set_layout; }
    [[nodiscard]] auto GetBindings() -> std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> & { return m_bindings; }

private:
    std::shared_ptr<RenderDevice> m_render_device;
    VkDescriptorSetLayout m_descriptor_set_layout;
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_bindings;
};

class RenderDescriptorWriter {
public:
    RenderDescriptorWriter(std::shared_ptr<RenderDescriptorSetLayout> set_layout, std::shared_ptr<RenderDescriptorPool> pool);

    auto WriteBuffer(uint32_t binding, VkDescriptorBufferInfo *buffer_info) -> RenderDescriptorWriter &;
    auto WriteImage(uint32_t binding, VkDescriptorImageInfo *image_info) -> RenderDescriptorWriter &;

    auto Build(VkDescriptorSet &set) -> bool;
    void Overwrite(VkDescriptorSet &set);

private:
    std::shared_ptr<RenderDescriptorSetLayout> m_set_layout;
    std::shared_ptr<RenderDescriptorPool> m_pool;
    const std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> &m_bindings;
    std::vector<VkWriteDescriptorSet> m_writes;
};

}// namespace saturn