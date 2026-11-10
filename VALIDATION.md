# Milestone 1 validation

Validated in the development workspace on 10 November 2026:

- Linux GCC 14.2.0: core library and test executable compile; CTest passes 1/1.
- Emscripten 4.0.10: Release build compiles and links the full application,
  producing HTML, JavaScript and WebAssembly. raylib uses PLATFORM_WEB and
  OpenGL ES 2.0. Its bundled stb_vorbis source emits one upstream warning.
- CMake 4.4.4 and Ninja 1.13.2 were used here.

Not yet verified:

- Native graphical build: Linux configuration stops because this workspace
  lacks X11 development headers/libraries. This is not a Windows build result.
- Browser runtime: local Chromium cannot launch because the workspace denies
  its process-singleton socket. Compilation succeeded, but rendering, input,
  focus recovery and resizing still require the README manual checks.
- Windows native compilation and execution: must be checked on the user's
  UCRT64 environment. The reported tools are recorded in README.md.
- Portfolio iframe integration, deployment, audio, persistence and touch input.

Milestone 1 is implemented but its runtime acceptance checks remain open.
Do not treat browser compilation alone as proof of a playable browser demo.
