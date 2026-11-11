#pragma once
#include "core/World.h"
#include <algorithm>
namespace farm {
struct OfflineSummary { double elapsed=0, credited=0, earnings=0; bool clockBackwards=false; };
inline OfflineSummary catchUp(World& world, std::int64_t now) {
    OfflineSummary result;
    if (now < 0) return result;
    const auto saved = world.savedAt();
    if (saved > 0 && now >= saved) {
        result.elapsed = static_cast<double>(now - saved);
        result.credited = std::min(result.elapsed, 8.0 * 3600);
        const double before = world.balance();
        world.tick(result.credited);
        result.earnings = world.balance() - before;
    } else result.clockBackwards = saved > now;
    // keep the later timestamp if the clock moves backwards
    world.markSaved(now);
    return result;
}
}
