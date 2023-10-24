#pragma once

#include <engine_pch.hpp>

#include <GLFW/glfw3.h>

namespace saturn {

class Window {
public:
    Window(int width, int height, std::string name);
    auto GetGlfwWindow() -> GLFWwindow *;
    ~Window();
    
    bool m_has_resized;

private:
    int m_width, m_height;
    std::string m_name;
    GLFWwindow *m_glfw_window;
};

}// namespace saturn