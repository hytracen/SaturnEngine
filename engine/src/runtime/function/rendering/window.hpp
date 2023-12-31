#pragma once

#include <engine_pch.hpp>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h> // 要在vulkan/vulkan.h后面，因为其需要用到vulkan.h中的一个宏定义

namespace saturn {

namespace rendering {
    
class Window {
public:
    Window(int width, int height, std::string name);
    ~Window();

    auto GetGlfwWindow() -> GLFWwindow * { return m_glfw_window; };
    // void CreateSurface(VkInstance instance, VkSurfaceKHR *surface);
    // void DestroySurface(VkInstance instance, VkSurfaceKHR *surface);

    bool m_has_resized;

private:
    int m_width, m_height;
    std::string m_name;
    GLFWwindow *m_glfw_window;
};

}  // namespace rendering


}// namespace saturn