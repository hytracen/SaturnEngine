#pragma once

#include <engine_pch.hpp>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

namespace saturn {

class RenderWindow {
public:
    RenderWindow(int width, int height, std::string name);
    ~RenderWindow();
    
    auto GetGlfwWindow() -> GLFWwindow * { return m_glfw_window; };
    // void CreateSurface(VkInstance instance, VkSurfaceKHR *surface);
    // void DestroySurface(VkInstance instance, VkSurfaceKHR *surface);

    bool m_has_resized;

private:
    int m_width, m_height;
    std::string m_name;
    GLFWwindow *m_glfw_window;
    
};

}// namespace saturn