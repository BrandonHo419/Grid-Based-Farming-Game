#include "core/World.h"
#include <algorithm>
#include <cmath>

void farm::World::tick(Input input) {
    if (!std::isfinite(input.x) || !std::isfinite(input.y)) return;
    input.x = std::clamp(input.x, -1.0, 1.0);
    input.y = std::clamp(input.y, -1.0, 1.0);
    const double length = std::hypot(input.x, input.y);
    if (length > 1.0) { input.x /= length; input.y /= length; }
    player_.x = std::clamp(player_.x + input.x * speed * tickSeconds,
                           radius, columns - radius);
    player_.y = std::clamp(player_.y + input.y * speed * tickSeconds,
                           radius, rows - radius);
}
