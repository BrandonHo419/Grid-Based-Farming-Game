# Grid-Based Farming Game

A small C++ farming game for desktop and browsers. This is Brandon Ho's rebuild
of the [original university group project](https://github.com/BrandonHo419/GridFarmGame).
See [CREDITS.md](CREDITS.md) for provenance and reuse restrictions.

## Current milestone

A 10-by-7 grid and a placeholder farmer controlled with WASD. Movement is
bounded, diagonal movement is normalised, and simulation pauses when unfocused.
The first tile is highlighted for the next milestone. Buying, crops and money
are not implemented yet. There are no external game assets to locate or load.

## Tools

- C++17 and CMake 3.25 or newer, using Ninja.
- Windows native builds: MSYS2 UCRT64 GCC; do not mix MINGW64 libraries.
- raylib 6.0, fetched automatically at a fixed commit by CMake.
- Browser builds: Emscripten SDK **4.0.10**, pinned for this milestone.
- Git and network access are needed for the initial dependency fetch.
- Python 3 is used only to serve the browser demo locally.

The user's reported native tools are GCC 16.2.0, CMake 4.4.5 and Ninja 1.13.2.
These are environment observations, not a claim that Windows was tested here.

## Windows desktop — MSYS2 UCRT64

Extract this project into a new directory, separate from the original game.
From the project root in **MSYS2 UCRT64**:

```bash
printf 'MSYSTEM=%s\n' "$MSYSTEM"
command -v gcc g++ cmake ninja
cmake -S . -B build/desktop -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
cmake --build build/desktop
ctest --test-dir build/desktop --output-on-failure
./build/desktop/grid_farm.exe
```

Expected: UCRT64; compilers, CMake and Ninja under `/ucrt64/bin`; tests pass;
a window displays a grid and gold farmer. Run from this shell so the UCRT64
runtime DLLs are available. Packaging a standalone executable is a later task.
Escape closes the desktop application.

## Browser setup — PowerShell

Use **PowerShell** for the Windows Emscripten SDK. It supplies its own compiler;
never point the web build at GCC. The following example stores tools under
`C:\dev` and assumes this project is at `C:\dev\Grid-Based-Farming-Game`.
If you extracted elsewhere, change only the project `Set-Location` line.

First check Python; install Python 3 before continuing if neither command works:

```powershell
Get-Command py, python -ErrorAction SilentlyContinue
py -3 --version
```

Install the pinned SDK in a new `C:\dev\emsdk` directory (do not clone over an
existing SDK). Git is already available on your machine.

```powershell
New-Item -ItemType Directory -Force C:\dev | Out-Null
Set-Location C:\dev
git clone --branch 4.0.10 --depth 1 https://github.com/emscripten-core/emsdk.git
Set-Location C:\dev\emsdk
.\emsdk.bat install 4.0.10
.\emsdk.bat activate 4.0.10
. .\emsdk_env.ps1
```

For each new PowerShell session, load the SDK environment again and add your
existing UCRT64 CMake and Ninja to the end of PATH. These are build tools; the
Emscripten toolchain selects `emcc` and `em++` explicitly.

```powershell
. C:\dev\emsdk\emsdk_env.ps1
$env:Path += ';C:\msys64\ucrt64\bin'
Set-Location C:\dev\Grid-Based-Farming-Game
emcc --version
emcmake cmake -S . -B build/web -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build/web
py -3 -m http.server 8000 --bind 127.0.0.1 --directory build/web
```

Open http://127.0.0.1:8000/grid_farm.html. If Python is available as `python`
only, substitute `python` for `py -3`. Do not open the HTML using a `file://`
URL. Keep `grid_farm.html`, `grid_farm.js` and `grid_farm.wasm` together.
No SDK files or downloaded build dependencies should be committed.

## Tests without graphics

In **MSYS2 UCRT64**, from the project root:

```bash
cmake -S . -B build/core -G Ninja -DGRIDFARM_BUILD_APP=OFF -DCMAKE_BUILD_TYPE=Debug
cmake --build build/core
ctest --test-dir build/core --output-on-failure
```

This mode does not fetch raylib. Tests check speed, diagonal movement, all four
boundaries, idle input and invalid numeric input. Checks also run in Release.

## Manual verification

1. Hold each WASD key: the farmer stops at every edge without leaving the grid.
2. Hold W+D: diagonal movement should not be faster than straight movement.
3. Resize the window or browser: the grid stays visible and centred.
4. In the browser, click the canvas to focus it; tab out and release movement
   keys, then return and click again. Movement should stop while unfocused,
   with no large jump or stuck movement on return.
5. Reload the browser page: the farmer starts on the highlighted tile.

Keyboard controls are the initial scope; touch controls are not implemented.
The eventual portfolio can embed the static browser output in an iframe.
Embedding and deployment have not been configured.

## Code layout

- `src/core`: framework-independent state and fixed-step movement.
- `src/app`: input, scheduling and rendering. Rendering does not update state.
- `src/main.cpp`: window ownership and platform loop selection.
- `tests`: standalone tests registered with CTest; no test framework dependency.
- `web`: browser shell, canvas focus and loading status.
- `assets`: asset reuse policy; currently no imported assets.

## Next milestone

Implement a complete one-crop loop: adjacent land purchases, planting cost,
growth, explicit harvesting and money. Test these rules independently of
animation before adding more crops, saving or progression.
