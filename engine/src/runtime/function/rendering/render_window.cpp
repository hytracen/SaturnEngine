#include "render_window.hpp"
namespace saturn {

Window::Window(int width, int height, std::string name) : m_width(width), m_height(height), m_name(std::move(name)) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);// 不创建OpenGL上下文
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_glfw_window = glfwCreateWindow(m_width, m_height, m_name.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(m_glfw_window, this);
    glfwSetFramebufferSizeCallback(m_glfw_window, [](GLFWwindow *in_window, int new_width, int new_height) {
        auto *window = reinterpret_cast<Window *>(glfwGetWindowUserPointer(in_window));
        window->m_width = new_width;
        window->m_height = new_height;
        window->m_has_resized = true;
    });

    ENGINE_LOG_INFO("Create \"{}\" window, width={} height={}", m_name, m_width, m_height);
}

auto Window::GetGlfwWindow() -> GLFWwindow * {
    return m_glfw_window;
}

Window::~Window() {
    glfwDestroyWindow(m_glfw_window);
    glfwTerminate();
}

}// namespace saturn