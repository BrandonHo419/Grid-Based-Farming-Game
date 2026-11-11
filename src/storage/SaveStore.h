#pragma once
#include "core/World.h"
#include <filesystem>
#include <string>

namespace farm {
class SaveStore {
public:
    static std::filesystem::path defaultPath();
    static bool save(const std::filesystem::path& path, const World& world, std::string& error);
    static bool load(const std::filesystem::path& path, World& world, std::string& error);
};
}
