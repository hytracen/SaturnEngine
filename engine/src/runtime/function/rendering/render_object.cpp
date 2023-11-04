#include "render_object.hpp"

#include <utility>

namespace saturn {

namespace rendering {

RenderObject::RenderObject(std::shared_ptr<Device> render_device, std::unique_ptr<resource::Model> model)
    : m_render_device(std::move(render_device)), m_model(std::move(model)) {
    CreateVertexBuffer();
    CreateIndexBuffer();
}

auto RenderObject::GetBindingDescriptions() -> std::vector<VkVertexInputBindingDescription> {
    std::vector<VkVertexInputBindingDescription> binding_descriptions(1);
    binding_descriptions[0].binding = 0;
    binding_descriptions[0].stride = sizeof(resource::Model::Vertex);
    binding_descriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return binding_descriptions;
}

auto RenderObject::GetAttributeDescriptions() -> std::vector<VkVertexInputAttributeDescription> {
    std::vector<VkVertexInputAttributeDescription> attribute_descriptions{};
    attribute_descriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(resource::Model::Vertex, m_position)});
    attribute_descriptions.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(resource::Model::Vertex, m_color)});
    attribute_descriptions.push_back({2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(resource::Model::Vertex, m_normal)});
    attribute_descriptions.push_back({3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(resource::Model::Vertex, m_uv)});

    return attribute_descriptions;
}

void RenderObject::CreateVertexBuffer() {
    auto vertices = m_model->m_vertices;

    rendering::Buffer staging_buffer{m_render_device, sizeof(vertices[0]), static_cast<uint32_t>(vertices.size()),
                                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
    staging_buffer.Map();
    staging_buffer.WriteToBuffer(vertices.data());
    staging_buffer.Unmap();

    m_vertex_buffer = std::make_shared<rendering::Buffer>(
            m_render_device, sizeof(vertices[0]), static_cast<uint32_t>(vertices.size()),
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    staging_buffer.CopyToBuffer(m_vertex_buffer);
}

void RenderObject::CreateIndexBuffer() {
    auto indices = m_model->m_indices;

    rendering::Buffer staging_buffer{m_render_device, sizeof(indices[0]), static_cast<uint32_t>(indices.size()),
                                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
    staging_buffer.Map();
    staging_buffer.WriteToBuffer(indices.data());
    staging_buffer.Unmap();

    m_index_buffer = std::make_shared<rendering::Buffer>(
            m_render_device, sizeof(indices[0]), static_cast<uint32_t>(indices.size()),
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    staging_buffer.CopyToBuffer(m_index_buffer);
}


}// namespace rendering

}// namespace saturn