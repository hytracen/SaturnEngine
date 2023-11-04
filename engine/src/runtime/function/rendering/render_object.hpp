#pragma once

#include <engine_pch.hpp>
#include <runtime/function/rendering/buffer.hpp>
#include <runtime/resource/model.hpp>

namespace saturn {
    
namespace rendering {

class RenderObject {
public:
    explicit RenderObject(std::shared_ptr<Device> render_device, std::unique_ptr<resource::Model> model);
    auto GetBindingDescriptions() -> std::vector<VkVertexInputBindingDescription>;
    auto GetAttributeDescriptions() -> std::vector<VkVertexInputAttributeDescription>;

    auto GetVertexBuffer() -> std::shared_ptr<Buffer> { return m_vertex_buffer; }
    auto GetIndexBuffer() -> std::shared_ptr<Buffer> { return m_index_buffer; }

    [[nodiscard]] auto GetVertices() const -> const std::vector<resource::Model::Vertex>& { return m_model->m_vertices; }
    [[nodiscard]] auto GetIndices() const -> const std::vector<uint32_t>& { return m_model->m_indices; }

private:
    void CreateVertexBuffer();
    void CreateIndexBuffer();

    std::shared_ptr<Device> m_render_device;
    std::unique_ptr<resource::Model> m_model;
    std::shared_ptr<Buffer> m_vertex_buffer;
    std::shared_ptr<Buffer> m_index_buffer;
};

}  // namespace rendering

}  // namespace saturn