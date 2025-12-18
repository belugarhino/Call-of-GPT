#pragma once

#include <raylib.h>

struct InputState {
    bool moveForward = false;
    bool moveBackward = false;
    bool moveLeft = false;
    bool moveRight = false;
    bool fire = false;
    bool pause = false;
    Vector2 lookDelta{}; // Mouse delta for yaw/pitch control.
};

inline InputState CaptureInput() {
    InputState input{};
    input.moveForward = IsKeyDown(KEY_W) || IsKeyDown(KEY_UP);
    input.moveBackward = IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN);
    input.moveLeft = IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT);
    input.moveRight = IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT);
    input.fire = IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_SPACE);
    input.pause = IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE);
    input.lookDelta = GetMouseDelta();
    return input;
}
