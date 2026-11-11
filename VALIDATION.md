# Validation

Checks run on Windows with UCRT64 GCC 16.2.0, CMake/Ninja and raylib 6.0.

## Native checks

- Debug desktop and headless builds pass.
- All four CTest suites pass: `core_rules`, `save_rules`, `offline_rules`, `ui_rules`.
- The optional desktop checks pass 38 assertions covering actions, settings, pause
  behavior, reduced-motion earnings, saves and repeated offline recovery.
- A scripted session covers 600 simulated seconds. This is accelerated testing,
  not a ten-minute human playthrough.
- Audio device checks pass for playback, mute, volume and focus behavior, including
  two complete 16-second music loops. Sound quality has not been assessed by listening.
- Native screenshots were inspected at 360x520, 900x520 and 1000x800, including
  settings. Short landscape windows use the side panel to keep the farm readable.

The runtime harness injects input directly, so it does not verify OS focus routing.
Its saves are isolated from the normal farm slot. The save tests cover round trips,
invalid input, failed writes, capped offline income and repeated-load protection.

Earlier Linux runs passed core tests and undefined-behavior checks. Earlier
Emscripten 4.0.10 builds compiled successfully; compilation alone is not a browser
runtime test.

## Portfolio integration

- Release WebAssembly build passes with Emscripten 4.0.10 on Windows.
- The portfolio generator check and all eight existing Python tests pass.
- Headless Chrome loaded the actual compiled game inside the portfolio iframe.
  Keyboard planting and worker hiring were verified in the rendered game.
- On-demand loading, canvas focus, unloading on close and a fresh farm on reopen
  were checked. No JavaScript runtime exceptions occurred.
- Screenshots were inspected at 1200x1000 and 390x844. The page has no horizontal
  overflow at the narrow size; the game keeps its 360px minimum inside its container.
- Browser progress is session-only. Persistence, Firefox/Safari compatibility,
  touch-only play and listening quality were not verified.

The compiled demo and site changes are local; nothing was committed or deployed.
The build still emits the upstream stb_vorbis pointer-comparison warning. Native
storage compilation emits the existing NOMINMAX redefinition warning. Neither
prevents the builds or tests from passing.
