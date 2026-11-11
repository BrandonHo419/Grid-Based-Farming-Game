# Grid-Based-Farming Game

A small idle farming game in C++17 and raylib. Plant crops, hire workers, buy land
and improve yields. The desktop version saves your farm and counts up to eight
hours of offline production. The browser demo is playable inside the portfolio.

Grid-Based-Farming Game is Brandon Ho's (me) rebuild of my University Group Project in OOP (Object-Oriented Programming)
[GridFarmGame](https://github.com/BrandonHo419/GridFarmGame).
See [CREDITS.md](CREDITS.md) for the original project and dependency notices.

## Playing

Start with $1000 and one free tile. Plots are permanent: harvesting restarts their
cycle without another seed cost. A hired worker handles every plot of its crop.
Buy new land beside an owned tile. Plant something before spending all your money
on land; restart if you get stuck.

| Crop | First plot | Cycle | Yield | Worker |
| --- | ---: | ---: | ---: | ---: |
| Tomatoes | 25 | 10s | 60 | 250 |
| Blueberries | 50 | 20s | 120 | 500 |
| Carrots | 15 | 6s | 36 | 150 |
| Beets | 35 | 14s | 84 | 350 |
| Strawberries | 40 | 16s | 96 | 400 |

Extra plots cost `base * 1.15^existing plots`. Yield upgrades multiply income by
1.25 and cost `base plot cost * 8 * 1.8^level`. Tending advances a crop by 10% of
its cycle, with a shared five-second cooldown. Worker movement is cosmetic.

| Control | Action |
| --- | --- |
| WASD | Move the farmer |
| Arrows / click | Select a tile |
| 1-5 / crop buttons | Select a crop |
| B / P / H | Buy land / plant / harvest |
| M / U / T | Hire a worker / upgrade / tend |
| Space | Pause or resume |
| F1 / Help | Controls |
| F2 / Settings | Sound and reduced motion |
| R, then Y or N | Restart or cancel |
| F5 / F9, then Y or N | Save / load on desktop |

Settings pause play while open. Q mutes, Left/Right adjusts music, Down/Up adjusts
effects and V toggles reduced motion. The controls are clickable too. Settings
last for the session. Sound starts after interaction and pauses when unfocused.
The game runs without an audio device.

## Desktop saving

Windows: `%LOCALAPPDATA%/Grid-Based-Farming-Game/idle.save`.
Linux: `$XDG_DATA_HOME/Grid-Based-Farming-Game/idle.save`, falling back to
`~/.local/share/Grid-Based-Farming-Game/idle.save`.

The game loads automatically and starts paused. Autosave runs every 30 seconds,
and normal close attempts a final save. A forced close can lose recent changes.
Use one game instance per save slot. The old `farm.save` is never imported or replaced.

Automated plots earn for up to eight hours away; manual crops only mature.
Repeated loads don't repeat the same offline reward. Backward clock changes award
nothing until the saved time is reached. Local clock or save editing is not prevented.
An unreadable save disables autosave to protect the file. F5 explicitly replaces it.
If a crash leaves an `idle.save.writing` folder, close all game instances before
removing that staging folder. Keep `idle.save`.

## Build on Windows

Install MSYS2 UCRT64 GCC, CMake 3.25+ and Ninja. From PowerShell in this repository:

```powershell
$env:Path = "C:\msys64\ucrt64\bin;$env:Path"
cmake -S . -B build/desktop -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
cmake --build build/desktop
ctest --test-dir build/desktop --output-on-failure
.\build\desktop\grid_farm.exe
```

Stop if a command fails. The first graphical build fetches the pinned raylib source.

For tests without graphics or a raylib download:

```powershell
cmake -S . -B build/core -G Ninja -DGRIDFARM_BUILD_APP=OFF -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
cmake --build build/core
ctest --test-dir build/core --output-on-failure
```

## Browser and portfolio

See [docs/PORTFOLIO.md](docs/PORTFOLIO.md) for the Emscripten build and embedding steps.
The browser version is a session-only demo: refreshing or closing it resets the
farm. Desktop save/load and closed-tab earnings are not available in the browser.
JavaScript and WebGL are required. Keyboard and mouse are recommended.

## Checks and screenshots

The optional runtime checks need working graphics and audio:

```powershell
cmake --build build/desktop --target desktop_checks
Push-Location build/desktop
.\desktop_checks.exe
Pop-Location
```

They use an isolated save folder, simulate 600 seconds of play and run two real
16-second music loops. Allow about 35 seconds. Screenshots and saves stay in the
new `runtime-check-*` output folder.

`grid_farm.exe --preview 1000 800` writes `farm-preview.png` in the working directory
and exits without touching saves. Use `--preview 360 520` for the compact layout;
append `settings` to show that panel.

For a manual check, plant and harvest, hire workers, upgrade, tend, resize the window,
and try Help and Settings while running and paused. Confirm mute, both volume
controls and reduced motion. Save, close and reopen to check offline progress.

[VALIDATION.md](VALIDATION.md) records the tested behavior and remaining limits.
[docs/DEVELOPMENT.md](docs/DEVELOPMENT.md) describes the code structure.
