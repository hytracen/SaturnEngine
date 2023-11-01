#pragma once

#include <engine_pch.hpp>

namespace saturn {

namespace resource {

class FileHelper {
public:
    [[nodiscard]] static auto ReadFile(const std::string &file_path_in_engine) -> std::vector<char>;
};

}



}// namespace saturn