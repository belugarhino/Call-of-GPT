# Call-of-GPT

Call-of-GPT is a barebones, expand-ready **3D first-person shooter** prototype inspired by Call of Duty. It uses a lightweight engine written in modern C++ and raylib for rendering, with clear entry points to extend gameplay, AI, and content.

## Features
- Minimal engine wrapper with fixed updates, pausing, and scene management.
- 3D first-person firing range: strafe with WASD, aim with the mouse, and shoot with mouse click/spacebar.
- Enemy sphere target that patrols a loop, deals contact damage, and scales difficulty with score.
- HUD crosshair, health bar, score, and survival timer.

## Controls
- **Move:** WASD or Arrow Keys
- **Fire:** Left Mouse Button or Spacebar
- **Pause:** `P` or `Esc`
- **Look:** Mouse (cursor captured while unpaused)

## Requirements
- CMake 3.20+
- A C++20 compiler (Visual Studio 2022 on Windows, Clang/GCC on Linux/macOS)
- Git (for fetching dependencies)

## Building on Windows (Visual Studio)
1. Open **x64 Native Tools Command Prompt for VS 2022**.
2. Clone the repository and create a build folder:
   ```bash
   git clone <repo-url>
   cd Call-of-GPT
   mkdir build && cd build
   ```
3. Configure the project (raylib will be fetched automatically):
   ```bash
   cmake -DCMAKE_BUILD_TYPE=Release ..
   ```
4. Build the executable:
   ```bash
   cmake --build . --config Release
   ```
5. Run the game:
   ```bash
   ./Release/CallOfGPT.exe
   ```

## Building on Linux/macOS
```bash
git clone <repo-url>
cd Call-of-GPT
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
./CallOfGPT
```

## Extending the prototype
- Add new scenes by subclassing `Scene` in `src/engine/Scene.h` and swapping them in `main.cpp`.
- Introduce AI behaviors in `src/game/GameScene.cpp` (e.g., roaming enemies, projectiles, or squads).
- Create new weapon types by extending the `Bullet` data and firing logic in `GameScene::updatePlayer`.

This project is intentionally minimal to encourage experimentation and iteration.
