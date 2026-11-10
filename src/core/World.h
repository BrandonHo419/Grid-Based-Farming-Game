#pragma once

namespace farm {
struct Input { double x = 0; double y = 0; };
struct Position { double x = 0.5; double y = 0.5; };
class World {
public:
    static constexpr int columns = 10;
    static constexpr int rows = 7;
    static constexpr double radius = 0.2;
    static constexpr double speed = 3.0;
    static constexpr double tickSeconds = 1.0 / 60.0;
    void tick(Input input);
    Position player() const { return player_; }
private:
    Position player_;
};
}
