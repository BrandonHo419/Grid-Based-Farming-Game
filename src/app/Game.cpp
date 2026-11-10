#include "app/Game.h"
#include "raylib.h"
#include <algorithm>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

void Game::frame() {
#ifdef __EMSCRIPTEN__
    const int width = EM_ASM_INT({ return Math.max(240, Math.floor(document.getElementById('game').clientWidth)); });
    const int height = EM_ASM_INT({ return Math.max(300, Math.floor(document.getElementById('game').clientHeight)); });
    if (width != GetScreenWidth() || height != GetScreenHeight()) SetWindowSize(width, height);
    const bool focused = EM_ASM_INT({ return !document.hidden && document.activeElement === Module.canvas; }) != 0;
#else
    const bool focused = IsWindowFocused();
#endif
    if (focused && focusedLastFrame_) {
        const farm::Input input{
            static_cast<double>(IsKeyDown(KEY_D) - IsKeyDown(KEY_A)),
            static_cast<double>(IsKeyDown(KEY_S) - IsKeyDown(KEY_W))};
        // Discard long stalls: changing tabs must not fast-forward the simulation.
        accumulator_ += std::clamp(static_cast<double>(GetFrameTime()), 0.0, 0.1);
        while (accumulator_ >= farm::World::tickSeconds) {
            world_.tick(input);
            accumulator_ -= farm::World::tickSeconds;
        }
    } else accumulator_ = 0;
    focusedLastFrame_ = focused;
    render();
}

void Game::render() const {
    const float width = static_cast<float>(GetScreenWidth());
    const float height = static_cast<float>(GetScreenHeight());
    const float tile = std::max(1.0f, std::min((width - 32) / farm::World::columns,
                                             (height - 130) / farm::World::rows));
    const float left = (width - tile * farm::World::columns) / 2;
    const float top = 90 + (height - 110 - tile * farm::World::rows) / 2;
    BeginDrawing();
    ClearBackground(Color{24, 36, 31, 255});
    DrawText("GRID-BASED FARMING GAME", 16, 16, width < 600 ? 16 : 24, RAYWHITE);
    DrawText("WASD: move | Click to focus", 16, 48, 14, Color{191, 210, 189, 255});
    DrawText("Milestone 1: movement prototype", 16, 68, 12, GRAY);
    for (int row = 0; row < farm::World::rows; ++row) {
        for (int col = 0; col < farm::World::columns; ++col) {
            Rectangle cell{left + col * tile, top + row * tile, tile - 2, tile - 2};
            DrawRectangleRec(cell, col == 0 && row == 0 ? Color{137, 103, 67, 255}
                                                                     : Color{58, 83, 61, 255});
        }
    }
    const auto player = world_.player();
    DrawCircleV(Vector2{left + static_cast<float>(player.x) * tile,
                       top + static_cast<float>(player.y) * tile},
                static_cast<float>(farm::World::radius) * tile, Color{241, 196, 98, 255});
    EndDrawing();
}
