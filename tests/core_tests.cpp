#include "core/World.h"
#include <cmath>
#include <iostream>
#include <limits>

int main() {
    int failures = 0;
    auto check = [&](bool valid, const char* name) {
        if (!valid) { std::cerr << "FAIL: " << name << '\n'; ++failures; }
    };
    auto close = [](double a, double b) { return std::abs(a - b) < 1e-9; };
    farm::World straight, diagonal;
    for (int i = 0; i < 60; ++i) { straight.tick({1, 0}); diagonal.tick({1, 1}); }
    check(close(straight.player().x, 3.5), "three tiles per second");
    check(close(std::hypot(diagonal.player().x - .5, diagonal.player().y - .5), 3), "no diagonal speed boost");
    for (int i = 0; i < 1000; ++i) straight.tick({1, 1});
    check(close(straight.player().x, 9.8) && close(straight.player().y, 6.8), "right and bottom bounds");
    for (int i = 0; i < 1000; ++i) straight.tick({-1, -1});
    check(close(straight.player().x, .2) && close(straight.player().y, .2), "left and top bounds");
    const auto before = straight.player();
    straight.tick({});
    straight.tick({std::numeric_limits<double>::quiet_NaN(), 0});
    check(close(straight.player().x, before.x) && close(straight.player().y, before.y), "idle and invalid input do not move");
    if (!failures) std::cout << "All movement checks passed.\n";
    return failures ? 1 : 0;
}
