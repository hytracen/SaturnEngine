#include "file_helper.hpp"
namespace saturn {

namespace resource {

auto FileHelper::ReadFile(const std::string &file_path_in_engine) -> std::vector<char> {
    std::string file_path = ENGINE_ROOT_DIR + file_path_in_engine;
    std::ifstream file{file_path, std::ios::ate | std::ios::binary};

    if (!file.is_open()) { throw std::runtime_error("failed to open file: " + file_path); }

    size_t file_size = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), file_size);

    file.close();
    return buffer;
}

}// namespace resource


}// namespace saturn