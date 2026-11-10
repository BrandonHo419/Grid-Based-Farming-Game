#pragma once
#include "core/World.h"

class Game {
public:
    void frame();
private:
    farm::World world_;
    double accumulator_ = 0;
    bool focusedLastFrame_ = false;
    void render() const;
};
