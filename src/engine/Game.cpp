#include "Game.h"

#include <raylib.h>

#include <algorithm>

Game::Game(GameConfig config) : config(std::move(config)) {}

void Game::setScene(std::unique_ptr<Scene> newScene) { scene = std::move(newScene); }

void Game::run() {
    InitWindow(config.screenWidth, config.screenHeight, config.windowTitle.c_str());
    SetTargetFPS(config.targetFPS);
    DisableCursor();

    constexpr float fixedStep = 1.0f / 120.0f;
    float accumulator = 0.0f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        accumulator = std::min(accumulator + dt, 0.25f);

        InputState input = CaptureInput();
        if (input.pause) {
            paused = !paused;
            if (paused) {
                EnableCursor();
            } else {
                DisableCursor();
            }
        }

        while (accumulator >= fixedStep) {
            if (!paused && scene) {
                scene->fixedUpdate(fixedStep, input);
            }
            accumulator -= fixedStep;
        }

        if (!paused && scene) {
            scene->update(dt, input);
        }

        if (scene) {
            BeginDrawing();
            scene->render();
            if (paused) {
                DrawRectangle(0, 0, config.screenWidth, config.screenHeight, Color{0, 0, 0, 120});
                DrawText("PAUSED", config.screenWidth / 2 - 80, config.screenHeight / 2 - 20, 40, RAYWHITE);
            }
            EndDrawing();
        }
    }

    EnableCursor();
    CloseWindow();
}
