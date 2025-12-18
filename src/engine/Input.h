#pragma once

#include <raylib.h>

struct InputState {
    bool moveForward = false;
    bool moveBackward = false;
    bool moveLeft = false;
    bool moveRight = false;
    bool fire = false;
    bool aim = false;
    bool slide = false;
    bool toggleMelee = false;
    int weaponHotkey = -1;
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
    input.aim = IsMouseButtonDown(MOUSE_RIGHT_BUTTON);
    input.slide = IsKeyPressed(KEY_LEFT_CONTROL) || IsKeyPressed(KEY_RIGHT_CONTROL);
    input.toggleMelee = IsKeyPressed(KEY_Q);

    if (IsKeyPressed(KEY_ONE)) input.weaponHotkey = 0;
    else if (IsKeyPressed(KEY_TWO)) input.weaponHotkey = 1;
    else if (IsKeyPressed(KEY_THREE)) input.weaponHotkey = 2;
    else if (IsKeyPressed(KEY_FOUR)) input.weaponHotkey = 3;
    else if (IsKeyPressed(KEY_FIVE)) input.weaponHotkey = 4;
    else if (IsKeyPressed(KEY_SIX)) input.weaponHotkey = 5;

    input.pause = IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE);
    input.lookDelta = GetMouseDelta();
    return input;
}
