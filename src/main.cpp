#include "app/Game.h"
#include "raylib.h"
#include <iostream>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace {
class Window {
public:
    Window() {
        SetConfigFlags(FLAG_WINDOW_RESIZABLE);
        InitWindow(1000, 800, "Grid-Based Farming Game");
        if (IsWindowReady()) SetWindowMinSize(240, 300);
    }
    ~Window() { if (IsWindowReady()) CloseWindow(); }
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
};
}
int main() {
    Window window;
    if (!IsWindowReady()) { std::cerr << "Unable to create the game window.\n"; return 1; }
    Game game;
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg([](void* context) {
        static_cast<Game*>(context)->frame();
    }, &game, 0, true);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose()) game.frame();
#endif
    return 0;
}
