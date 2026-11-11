#include "app/Game.h"
#include "raylib.h"
#include <iostream>
#include <cstdlib>
#include <string>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace {
class Window {
public:
    Window(int width,int height) {
        SetConfigFlags(FLAG_WINDOW_RESIZABLE);
        InitWindow(width, height, "Grid-Based-Farming Game");
        if (IsWindowReady()) SetWindowMinSize(360, 520);
    }
    ~Window() { if (IsWindowReady()) CloseWindow(); }
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
};
}
int main(int argc,char** argv) {
    const bool preview=argc>=2 && std::string(argv[1])=="--preview";
    const int width=preview && argc>=4?std::clamp(std::atoi(argv[2]),360,1920):1000;
    const int height=preview && argc>=4?std::clamp(std::atoi(argv[3]),520,1080):800;
    Window window(width,height);
    if (!IsWindowReady()) { std::cerr << "Unable to create the game window.\n"; return 1; }
    Game game(preview,preview && argc>=5 && std::string(argv[4])=="settings");
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg([](void* context) {
        static_cast<Game*>(context)->frame();
    }, &game, 0, true);
#else
    SetTargetFPS(60);
    if(preview) {
        for(int i=0;i<5;++i) game.frame();
        TakeScreenshot("farm-preview.png");
        return 0;
    }
    while (!WindowShouldClose()) game.frame();
    if (!game.saveOnClose()) return 1;
#endif
    return 0;
}
