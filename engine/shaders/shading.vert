#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 light_view_proj;
}
ubo;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec3 in_normal;
layout(location = 3) in vec2 in_texcoord;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec2 frag_tex_coord;
layout(location = 2) out vec3 frag_normal;
layout(location = 3) out vec4 light_proj_pos;

layout(location = 4) out vec3 world_position;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(in_position, 1.0);
    world_position = in_position;

    frag_color = in_color;
    frag_tex_coord = in_texcoord;

    //TODO(处理非均匀缩放问题)
    frag_normal = (ubo.model * vec4(in_normal, 0.0)).xyz;
    light_proj_pos = ubo.light_view_proj * ubo.model * vec4(in_position, 1.0);
}
