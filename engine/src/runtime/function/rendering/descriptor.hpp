#pragma once

#include <engine_pch.hpp>
#include <utility>

#include "device.hpp"

namespace saturn {

namespace rendering {

class DescriptorPool {
public:
    //-----------------------------------Builder-------------------------------------
    class Builder {
    public:
        explicit Builder(std::shared_ptr<Device> render_device) : m_render_device{std::move(render_device)} {}

        auto AddPoolSize(VkDescriptorType descriptor_type, uint32_t count) -> Builder &;
        auto SetPoolFlags(VkDescriptorPoolCreateFlags flags) -> Builder &;
        // auto SetMaxSets(uint32_t count) -> Builder &;

        [[nodiscard]] auto Build() const -> std::shared_ptr<DescriptorPool>;

    private:
        std::shared_ptr<Device> m_render_device;
        std::vector<VkDescriptorPoolSize> m_pool_sizes{};
        // uint32_t m_max_sets = 1000;
        uint32_t m_current_sets_count = 0;
        VkDescriptorPoolCreateFlags m_pool_flags = 0;
    };
    //-------------------------------------------------------------------------------


    DescriptorPool(
            std::shared_ptr<Device> render_device,
            uint32_t max_sets,
            VkDescriptorPoolCreateFlags pool_flags,
            const std::vector<VkDescriptorPoolSize> &pool_sizes);
    ~DescriptorPool();
    DescriptorPool(const DescriptorPool &) = delete;
    auto operator=(const DescriptorPool &) -> DescriptorPool & = delete;


    [[nodiscard]] auto GetDescriptorPool() const -> VkDescriptorPool { return m_descriptor_pool; };
    [[nodiscard]] auto GetDevice() const -> std::shared_ptr<Device> { return m_render_device; }

    auto AllocateDescriptor(
            VkDescriptorSetLayout descriptor_set_layout, VkDescriptorSet &descriptor_set) const -> bool;

    void FreeDescriptors(std::vector<VkDescriptorSet> &descriptor_sets) const;

    void ResetPool();

private:
    std::shared_ptr<Device> m_render_device;
    VkDescriptorPool m_descriptor_pool;

    friend class DescriptorWriter;
};

class DescriptorSetLayout {
public:
    //-----------------------------------Builder-------------------------------------
    class Builder {
    public:
        explicit Builder(std::shared_ptr<Device> render_device) : m_render_device{std::move(render_device)} {}

        auto AddBinding(uint32_t binding, VkDescriptorType descriptor_type, VkShaderStageFlags stage_flags, uint32_t count = 1) -> Builder &;
        [[nodiscard]] auto Build() const -> std::unique_ptr<DescriptorSetLayout>;

    private:
        std::shared_ptr<Device> m_render_device;
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
    };
    //-------------------------------------------------------------------------------

    DescriptorSetLayout(std::shared_ptr<Device> render_device, const std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> &bindings);
    ~DescriptorSetLayout();
    DescriptorSetLayout(const DescriptorSetLayout &) = delete;
    auto operator=(const DescriptorSetLayout &) -> DescriptorSetLayout & = delete;

    [[nodiscard]] auto GetDescriptorSetLayout() const -> VkDescriptorSetLayout { return m_descriptor_set_layout; }
    [[nodiscard]] auto GetBindings() -> std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> & { return m_bindings; }

private:
    std::shared_ptr<Device> m_render_device;
    VkDescriptorSetLayout m_descriptor_set_layout;
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_bindings;
};

class DescriptorWriter {
public:
    DescriptorWriter(std::shared_ptr<DescriptorSetLayout> set_layout, std::shared_ptr<DescriptorPool> pool);

    auto WriteBuffer(uint32_t binding, VkDescriptorBufferInfo *buffer_info) -> DescriptorWriter &;
    auto WriteImage(uint32_t binding, VkDescriptorImageInfo *image_info) -> DescriptorWriter &;

    auto Build(VkDescriptorSet &set) -> bool;
    void Overwrite(VkDescriptorSet &set);

private:
    std::shared_ptr<DescriptorSetLayout> m_set_layout;
    std::shared_ptr<DescriptorPool> m_pool;
    const std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> &m_bindings;
    std::vector<VkWriteDescriptorSet> m_writes;
};

}  // namespace rendering

}// namespace saturn