#pragma once
#include "core/World.h"
#include <algorithm>

struct TileSelection {
    int column = 0;
    int row = 0;
    void move(int horizontal, int vertical) {
        column = std::clamp(column + std::clamp(horizontal, -1, 1), 0, farm::World::columns - 1);
        row = std::clamp(row + std::clamp(vertical, -1, 1), 0, farm::World::rows - 1);
    }
};
