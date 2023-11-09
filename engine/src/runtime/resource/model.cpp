#include "model.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace std {
template<>
struct hash<saturn::resource::Model::Vertex> {
    auto operator()(saturn::resource::Model::Vertex const &vertex) const -> size_t {
        size_t seed = 0;
        saturn::HashCombine(seed, vertex.m_position, vertex.m_color, vertex.m_normal, vertex.m_uv);
        return seed;
    }
};
}// namespace std

namespace saturn {

namespace resource {

//-----------------------------------vertex-----------------------------------
auto Model::Vertex::GetBindingDescriptions() -> std::vector<VkVertexInputBindingDescription> {
    std::vector<VkVertexInputBindingDescription> binding_descriptions(1);
    binding_descriptions[0].binding = 0;
    binding_descriptions[0].stride = sizeof(Vertex);
    binding_descriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return binding_descriptions;
}

auto Model::Vertex::GetAttributeDescriptions() -> std::vector<VkVertexInputAttributeDescription> {
    std::vector<VkVertexInputAttributeDescription> attribute_descriptions{};

    attribute_descriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, m_position)});
    attribute_descriptions.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, m_color)});
    attribute_descriptions.push_back({2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, m_normal)});
    attribute_descriptions.push_back({3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, m_uv)});

    return attribute_descriptions;
}
//----------------------------------------------------------------------------

Model::Model(const std::string &file_path) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, file_path.c_str())) {
        throw std::runtime_error(warn + err);
    }

    m_vertices.clear();
    m_indices.clear();

    std::unordered_map<Vertex, uint32_t> unique_vertices{};
    for (const auto &shape: shapes) {
        for (const auto &index: shape.mesh.indices) {
            Vertex vertex{};

            if (index.vertex_index >= 0) {
                vertex.m_position = {
                        attrib.vertices[3 * index.vertex_index + 0],
                        attrib.vertices[3 * index.vertex_index + 1],
                        attrib.vertices[3 * index.vertex_index + 2],
                };

                vertex.m_color = {
                        attrib.colors[3 * index.vertex_index + 0],
                        attrib.colors[3 * index.vertex_index + 1],
                        attrib.colors[3 * index.vertex_index + 2],
                };
            }

            if (index.normal_index >= 0) {
                vertex.m_normal = {
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2],
                };
            }

            if (index.texcoord_index >= 0) {
                vertex.m_uv = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1], // vulkan的uv原点是左上角，OBJ模型文件的uv原点是左下角，因此需要进行反转
                };
            }

            if (!unique_vertices.contains(vertex)) {
                unique_vertices[vertex] = static_cast<uint32_t>(m_vertices.size());
                m_vertices.push_back(vertex);
            }
            m_indices.push_back(unique_vertices[vertex]);
        }
    }

    // 将模型放缩到标准立方体中
    float max_val = 0.0f;

    for (auto &vertex: m_vertices) {
        max_val = std::max(max_val, vertex.m_position.x);
        max_val = std::max(max_val, vertex.m_position.y);
        max_val = std::max(max_val, vertex.m_position.z);
    }

    for (auto &vertex: m_vertices) {
        vertex.m_position /= max_val;
        // 下面只针对japanese_temple.obj
        vertex.m_position *= 2.0f;
        vertex.m_position.y -= 1.0f;
    }

    ENGINE_LOG_INFO("Load model: {}\n vertices count:{}", file_path, m_vertices.size());
}

}  // namespace resource

}// namespace saturn