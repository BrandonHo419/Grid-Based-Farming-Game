# Development notes

The game is complete.

## How it's organised

- `src/core`: farm rules, crop data, offline progress and the save format.
- `src/app`: input, layout, drawing, worker animation and audio.
- `src/storage`: desktop save files and replacement writes.
- `tests`: core rules, saves, offline progress, layout and optional desktop checks.
- `web/shell.html`: the browser page wrapped around the compiled game.

`GameState` owns production. Drawing and worker movement don't award income.
Clocks, input and storage stay outside the simulation. The desktop slot uses
`GRID_IDLE_SAVE 2`; version 1 loads without an initial offline reward. Catch-up is
capped at eight hours and saved before its earnings are shown.

## Build tools

Use C++17, CMake 3.25+, Ninja, raylib 6.0 and Emscripten 4.0.10. CMake pins raylib
to `dbc56a87da87d973a9c5baa4e7438a9d20121d28`. On Windows, use MSYS2 UCRT64 GCC.
Keep native and browser builds in separate directories. Don't mix UCRT64 and
MINGW64 libraries.

