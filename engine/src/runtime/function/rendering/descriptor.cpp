#include "descriptor.hpp"

#include <utility>


namespace saturn {

namespace rendering {

// *************** Descriptor Pool *********************

DescriptorPool::DescriptorPool(
        std::shared_ptr<Device> render_device,
        uint32_t max_sets,
        VkDescriptorPoolCreateFlags pool_flags,
        const std::vector<VkDescriptorPoolSize> &pool_sizes)
    : m_render_device{std::move(render_device)} {

    VkDescriptorPoolCreateInfo descriptor_pool_info{};
    descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    descriptor_pool_info.pPoolSizes = pool_sizes.data();
    descriptor_pool_info.maxSets = max_sets;
    descriptor_pool_info.flags = pool_flags;

    if (vkCreateDescriptorPool(m_render_device->GetVkDevice(), &descriptor_pool_info, nullptr, &m_descriptor_pool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

DescriptorPool::~DescriptorPool() {
    vkDestroyDescriptorPool(m_render_device->GetVkDevice(), m_descriptor_pool, nullptr);
}

auto DescriptorPool::AllocateDescriptor(VkDescriptorSetLayout descriptor_set_layout, VkDescriptorSet &descriptor_set) const -> bool {
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = m_descriptor_pool;
    alloc_info.pSetLayouts = &descriptor_set_layout;
    alloc_info.descriptorSetCount = 1;

    return vkAllocateDescriptorSets(m_render_device->GetVkDevice(), &alloc_info, &descriptor_set) == VK_SUCCESS;
}

void DescriptorPool::FreeDescriptors(std::vector<VkDescriptorSet> &descriptor_sets) const {
    vkFreeDescriptorSets(
            m_render_device->GetVkDevice(),
            m_descriptor_pool,
            static_cast<uint32_t>(descriptor_sets.size()),
            descriptor_sets.data());
}

void DescriptorPool::ResetPool() {
    vkResetDescriptorPool(m_render_device->GetVkDevice(), m_descriptor_pool, 0);
}

// *************** Descriptor Set Layout Builder *********************

auto DescriptorSetLayout::Builder::AddBinding(
        uint32_t binding_index,
        VkDescriptorType descriptor_type,
        VkShaderStageFlags stage_flags,
        uint32_t count) -> DescriptorSetLayout::Builder & {
    SATURN_ASSERT(!bindings.contains(binding_index), "Binding already in use");
    VkDescriptorSetLayoutBinding layout_binding{};
    layout_binding.binding = binding_index;
    layout_binding.descriptorType = descriptor_type;
    layout_binding.descriptorCount = count;
    layout_binding.stageFlags = stage_flags;
    bindings[binding_index] = layout_binding;
    return *this;
}

auto DescriptorSetLayout::Builder::Build() const -> std::unique_ptr<DescriptorSetLayout> {
    return std::make_unique<DescriptorSetLayout>(m_render_device, bindings);
}

// *************** Descriptor Set Layout *********************

DescriptorSetLayout::DescriptorSetLayout(
        std::shared_ptr<Device> render_device, const std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> &bindings)
    : m_render_device{std::move(render_device)}, m_bindings{bindings} {

    std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings{};
    set_layout_bindings.reserve(bindings.size());
    for (auto kv: bindings) {
        set_layout_bindings.push_back(kv.second);
    }

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info{};
    descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_info.bindingCount = static_cast<uint32_t>(set_layout_bindings.size());
    descriptor_set_layout_info.pBindings = set_layout_bindings.data();

    if (vkCreateDescriptorSetLayout(m_render_device->GetVkDevice(), &descriptor_set_layout_info, nullptr, &m_descriptor_set_layout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }
}

DescriptorSetLayout::~DescriptorSetLayout() {
    vkDestroyDescriptorSetLayout(m_render_device->GetVkDevice(), m_descriptor_set_layout, nullptr);
}

// *************** Descriptor Pool Builder *********************

auto DescriptorPool::Builder::AddPoolSize(
        VkDescriptorType descriptor_type, uint32_t count) -> DescriptorPool::Builder & {
    m_pool_sizes.push_back({descriptor_type, count});
    m_current_sets_count += count;
    return *this;
}

auto DescriptorPool::Builder::SetPoolFlags(
        VkDescriptorPoolCreateFlags flags) -> DescriptorPool::Builder & {
    m_pool_flags = flags;
    return *this;
}

// /**
//  * 如果不设置，则为默认大小
//  */
// auto RenderDescriptorPool::Builder::SetMaxSets(uint32_t count) -> RenderDescriptorPool::Builder & {
//     m_max_sets = count;
//     return *this;
// }

auto DescriptorPool::Builder::Build() const -> std::shared_ptr<DescriptorPool> {
    SATURN_ASSERT(m_current_sets_count > 0, "Current descriptorsets count is zero!")
    return std::make_shared<DescriptorPool>(m_render_device, m_current_sets_count, m_pool_flags, m_pool_sizes);
}

// *************** Descriptor Writer *********************

DescriptorWriter::DescriptorWriter(std::shared_ptr<DescriptorSetLayout> set_layout, std::shared_ptr<DescriptorPool> pool)
    : m_set_layout{set_layout}, m_pool{std::move(pool)}, m_bindings(set_layout->GetBindings()) {}

auto DescriptorWriter::WriteBuffer(
        uint32_t binding, VkDescriptorBufferInfo *buffer_info) -> DescriptorWriter & {
    SATURN_ASSERT(m_bindings.count(binding) == 1, "Layout does not contain specified binding");

    const auto &binding_description = m_bindings.at(binding);

    SATURN_ASSERT(binding_description.descriptorCount == 1, "Binding single descriptor info, but binding expects multiple");

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorType = binding_description.descriptorType;
    write.dstBinding = binding;
    write.pBufferInfo = buffer_info;
    write.descriptorCount = 1;

    m_writes.push_back(write);
    return *this;
}

auto DescriptorWriter::WriteImage(
        uint32_t binding, VkDescriptorImageInfo *image_info) -> DescriptorWriter & {
    SATURN_ASSERT(m_bindings.count(binding) == 1, "Layout does not contain specified binding");

    const auto &binding_description = m_bindings.at(binding);

    SATURN_ASSERT(binding_description.descriptorCount == 1, "Binding single descriptor info, but binding expects multiple");

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorType = binding_description.descriptorType;
    write.dstBinding = binding;
    write.pImageInfo = image_info;
    write.descriptorCount = 1;

    m_writes.push_back(write);
    return *this;
}

auto DescriptorWriter::Build(VkDescriptorSet &set) -> bool {
    bool success = m_pool->AllocateDescriptor(m_set_layout->GetDescriptorSetLayout(), set);
    if (!success) {
        ENGINE_LOG_ERROR("Can't allocate descriptor");
        return false;
    }
    Overwrite(set);
    return true;
}

void DescriptorWriter::Overwrite(VkDescriptorSet &set) {
    for (auto &write: m_writes) {
        write.dstSet = set;
    }
    vkUpdateDescriptorSets(m_pool->GetDevice()->GetVkDevice(), m_writes.size(), m_writes.data(), 0, nullptr);
}


}

}// namespace saturn