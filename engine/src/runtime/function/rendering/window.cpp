#include "window.hpp"

namespace saturn {

namespace rendering {

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

Window::~Window() { glfwDestroyWindow(m_glfw_window); }

// void Window::CreateSurface(VkInstance instance, VkSurfaceKHR *surface) {
//     if (glfwCreateWindowSurface(instance, m_glfw_window, nullptr, surface) != VK_SUCCESS) {
//         throw std::runtime_error("failed to create window surface!");
//     }
// }

// void Window::DestroySurface(VkInstance instance, VkSurfaceKHR *surface) {
//     vkDestroySurfaceKHR(instance, *surface, nullptr);
// }

}// namespace rendering


}// namespace saturn