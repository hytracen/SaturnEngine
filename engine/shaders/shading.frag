#version 450

layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform sampler2D shadow_map_sampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 light_proj_pos;

layout(location = 0) out vec4 outColor;

float CalculateShadowAtPos(vec4 in_light_proj_pos, vec2 offset) {
    // 执行透视除法
    vec3 light_proj_coords = in_light_proj_pos.xyz / in_light_proj_pos.w;
    // 变换到[0,1]的范围
    vec2 shadow_tex_uv = light_proj_coords.xy * 0.5 + vec2(0.5, 0.5);
    // 反转y轴
    shadow_tex_uv.y = -shadow_tex_uv.y;
    // 取得最近点的深度(使用[0,1]范围下的fragPosLight当坐标)
    float closest_depth = texture(shadow_map_sampler, shadow_tex_uv + offset).r;
    // 取得当前片段在光源视角下的深度
    float current_depth = light_proj_coords.z;
    // 检查当前片段是否在阴影中
    float shadow = current_depth - 0.005 >= closest_depth ? 0.90 : 0.0;

    return shadow;
}

float Pcf(vec4 in_light_proj_pos) {
    ivec2 tex_dim = textureSize(shadow_map_sampler, 0);
    float scale = 1.0;
    float dx = scale * 1.0 / float(tex_dim.x);
    float dy = scale * 1.0 / float(tex_dim.y);

    float shadow_factor = 0.0;
    int count = 0;
    int range = 1;

    for (int x = -range; x <= range; x++) {
        for (int y = -range; y <= range; y++) {
            shadow_factor += CalculateShadowAtPos(in_light_proj_pos, vec2(dx * x, dy * y));
            count++;
        }
    }
    return shadow_factor / count;
}

void main() { outColor = vec4((1 - Pcf(light_proj_pos)) * texture(texSampler, fragTexCoord).rgb, 1.0); }
