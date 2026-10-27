# MyApp

Minimal C++20 project scaffold that vendors, via CMake `FetchContent`:

- [raylib](https://github.com/raysan5/raylib) — windowing / rendering
- [Dear ImGui](https://github.com/ocornut/imgui) — UI, using the
  [rlImGui](https://github.com/raylib-extras/rlImGui) raylib backend
- [Lua](https://github.com/lua/lua) — lua scripting
- [nlohmann/json](https://github.com/nlohmann/json) — JSON

All sources under `src/` are picked up automatically and compiled into
one executable. `src/main.cpp` is currently a minimal program that uses all
four libraries at once.

## Prerequisites

- CMake 3.20+
- Git (CMake shells out to it to fetch dependencies)
- A C++20 compiler:
  - Windows: Visual Studio 2022 (or the Build Tools for it)
  - Linux: GCC 11+ or Clang 14+
  - macOS: Xcode command line tools
- **Linux only** — raylib needs some system dev packages to build:
  ```bash
  sudo apt install libgl1-mesa-dev libx11-dev libxrandr-dev \
                    libxinerama-dev libxcursor-dev libxi-dev libwayland-dev libxkbcommon-dev
  ```
- An internet connection the *first* time you configure (dependencies
  are cloned into `build/_deps/` and cached there — later configures
  are offline).

## Windows — generate and use the .sln

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
```

This clones and builds raylib/imgui/rlImGui/lua/json, then generates
`build/MyApp.sln` (inside `build` folder). Open it in Visual Studio and hit Run — `MyApp` is
already set as the startup project. Or build from the command line:

```bat
cmake --build build --config Release
```

The executable ends up in `build/bin/Release/MyApp.exe`.

## Linux / macOS

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/bin/MyApp
```

(Pass `-G Ninja` to the first command if you have Ninja installed, for
faster builds.)

## Project layout

- `src/main.cpp` contains application startup, the render loop, and wiring.
- `src/data/CsvGyroLoader.*` validates and normalizes the CSV recording.
- `src/data/DataPipeline.h` provides the typed filter pipeline used by data
  preprocessing.
- `src/data/CharlesArm.*` converts gyro samples into the render-facing
  `FrameTrack`.
- `src/ui/` contains playback controls, camera controls, trails, and drawing.

Add more `.cpp`/`.c` files anywhere under `src/` and they are picked up
automatically on the next CMake configure (re-run `cmake -S . -B build` if
you add a new file so it is detected).

## Architecture diagrams

The PlantUML diagrams in [`docs/architecture/`](docs/architecture/) describe the
current implementation:

- [`context.puml`](docs/architecture/context.puml) — system context
- [`container.puml`](docs/architecture/container.puml) — logical containers
- [`component.puml`](docs/architecture/component.puml) — application components
- [`ingestion-sequence.puml`](docs/architecture/ingestion-sequence.puml) — CSV ingestion and frame-track preparation
- [`frame-sequence.puml`](docs/architecture/frame-sequence.puml) — one playback frame from user action to rendered showcase

The C4 diagrams use the standard C4-PlantUML library includes.
