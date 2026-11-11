#pragma once
#include "core/World.h"
#include <string>

namespace farm {
class SaveCodec {
public:
    static constexpr std::size_t maxBytes = 16384;
    static std::string encode(const World& world);
    // leave the world alone on failure; the error can be shown in the UI
    static bool decode(const std::string& text, World& world, std::string& error);
};
}
