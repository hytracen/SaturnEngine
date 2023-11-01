#pragma once
#include <engine_pch.hpp>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

namespace saturn {

namespace resource {

class Model {
public:
    struct Vertex {
        glm::vec3 m_position{};
        glm::vec3 m_color{};
        glm::vec3 m_normal{};
        glm::vec2 m_uv{};

        static auto GetBindingDescriptions() -> std::vector<VkVertexInputBindingDescription>;
        static auto GetAttributeDescriptions() -> std::vector<VkVertexInputAttributeDescription>;

        auto operator==(const Vertex &other) const -> bool {
            return m_position == other.m_position && m_color == other.m_color && m_normal == other.m_normal &&
                   m_uv == other.m_uv;
        }
    };


    explicit Model(const std::string &file_path);

    std::vector<Vertex> m_vertices{};
    std::vector<uint32_t> m_indices{};
};

}

template<typename T, typename... Rest>
void HashCombine(std::size_t &seed, const T &v, const Rest &...rest) {
    seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    (HashCombine(seed, rest), ...);
};

}// namespace saturn