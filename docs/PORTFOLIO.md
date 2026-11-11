# Portfolio demo

The game also runs in a WebAssembly iframe on the static
[portfolio](https://github.com/BrandonHo419/portfolio-preview). It needs no backend.
The portfolio loads the game when Play Grid-Based-Farming Game is opened and unloads it when
closed. A separate-tab link is available for a larger view.

Browser progress lasts for the open session only. Reloading, closing the panel or
closing the tab resets the farm. Desktop persistence is not part of this demo.

## Build

Install Emscripten SDK 4.0.10. Keep it outside the repository. The commands below
assume it is in the sibling `emsdk` folder. CMake and Ninja come from UCRT64;
Emscripten supplies the browser compiler.

From PowerShell in the game repository:

```powershell
$env:Path = "C:\msys64\ucrt64\bin;$env:Path"
$env:EMSDK = (Resolve-Path ../emsdk).Path
$env:EM_CONFIG = Join-Path $env:EMSDK '.emscripten'
$env:EM_CACHE = Join-Path $PWD 'build/emscripten-cache'
& ../emsdk/upstream/emscripten/emcmake.bat cmake -S . -B build/web -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build/web
```

Stop after any failed command. The SDK must already be installed and activated;
these commands do not change PowerShell execution policy. For an offline build,
reuse downloaded raylib with
`-DFETCHCONTENT_SOURCE_DIR_RAYLIB="$PWD/build/desktop/_deps/raylib-src"` during
configuration and copy the SDK's existing `upstream/emscripten/cache` into the
local `build/emscripten-cache` folder before building.

## Copy to the portfolio

The portfolio serves committed files, so it needs the generated HTML, JavaScript
and WebAssembly, not the C++ source or SDK. From the game repository:

```powershell
python tools/package_web.py ../portfolio-preview/demos/grid-based-farming-game --raylib build/desktop/_deps/raylib-src
```

Use `--raylib build/web/_deps/raylib-src` if the browser build fetched its own
raylib source. The packager also copies credits and upstream licence-bearing files.
It replaces named output files but does not delete other files in the destination.

The portfolio's farm entry lives in `data/projects.json`. Its inline player belongs
in `templates/projects.html`; `script.js` controls loading and unloading.
The screenshot is `assets/grid-based-farming-game-preview.png`.

From the portfolio repository:

```powershell
python -B tools/render_projects.py
python -B tools/render_projects.py --check
python -B -m unittest discover -s tests -v
python -m http.server 8000 --bind 127.0.0.1
```

