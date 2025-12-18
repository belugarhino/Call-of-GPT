#include "GameScene.h"

#include <raymath.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace {
constexpr float kMaxPitch = 89.0f * DEG2RAD;
constexpr float kMouseSensitivity = 0.0035f;

Vector3 ForwardFromAngles(float yaw, float pitch) {
    float cosPitch = std::cos(pitch);
    return {std::cos(yaw) * cosPitch, std::sin(pitch), std::sin(yaw) * cosPitch};
}
}

GameScene::GameScene(const GameConfig &config) : config(config) {
    player.position = {0.0f, 1.8f, -6.0f};
    player.camera.position = player.position;
    player.camera.target = Vector3Add(player.position, Vector3{0.0f, 1.6f, 1.0f});
    player.camera.up = {0.0f, 1.0f, 0.0f};
    player.camera.fovy = 75.0f;
    player.camera.projection = CAMERA_PERSPECTIVE;
    resetEnemy();
}

void GameScene::update(float dt, const InputState &input) {
    timeAlive += dt;
    muzzleFlashTimer = std::max(0.0f, muzzleFlashTimer - dt);
    respawnTimer = std::max(0.0f, respawnTimer - dt);
    player.fireTimer = std::max(0.0f, player.fireTimer - dt);

    updatePlayer(dt, input);
    updateEnemy(dt);
    updateBullets(dt);

    if (enemy.health <= 0 && respawnTimer <= 0.0f) {
        resetEnemy();
    }
}

void GameScene::fixedUpdate(float, const InputState &) {}

void GameScene::render() {
    ClearBackground(Color{12, 12, 16, 255});

    BeginMode3D(player.camera);
    drawWorld();
    EndMode3D();

    drawHUD();
}

void GameScene::updatePlayer(float dt, const InputState &input) {
    player.yaw += input.lookDelta.x * kMouseSensitivity;
    player.pitch = std::clamp(player.pitch - input.lookDelta.y * kMouseSensitivity, -kMaxPitch, kMaxPitch);

    Vector3 forward = ForwardFromAngles(player.yaw, player.pitch);
    Vector3 flatForward = {forward.x, 0.0f, forward.z};
    if (Vector3Length(flatForward) > 0.001f) flatForward = Vector3Normalize(flatForward);
    Vector3 right = Vector3CrossProduct(flatForward, {0.0f, 1.0f, 0.0f});
    if (Vector3Length(right) > 0.001f) right = Vector3Normalize(right);

    Vector3 move = {0.0f, 0.0f, 0.0f};
    if (input.moveForward) move = Vector3Add(move, flatForward);
    if (input.moveBackward) move = Vector3Subtract(move, flatForward);
    if (input.moveLeft) move = Vector3Subtract(move, right);
    if (input.moveRight) move = Vector3Add(move, right);

    if (Vector3Length(move) > 0.001f) {
        move = Vector3Normalize(move);
        player.position = Vector3Add(player.position, Vector3Scale(move, player.speed * dt));
    }

    // Keep player above ground plane.
    player.position.y = player.height;

    player.camera.position = player.position;
    player.camera.target = Vector3Add(player.position, forward);

    if (input.fire && player.fireTimer <= 0.0f) {
        Bullet bullet{};
        bullet.position = Vector3Add(player.position, {0.0f, -0.1f, 0.0f});
        bullet.velocity = Vector3Scale(forward, 65.0f);
        bullet.lifetime = 2.5f;
        bullets.push_back(bullet);

        muzzleFlashTimer = 0.08f;
        player.fireTimer = player.fireCooldown;
    }
}

void GameScene::updateBullets(float dt) {
    for (auto &bullet : bullets) {
        bullet.lifetime -= dt;
        bullet.position = Vector3Add(bullet.position, Vector3Scale(bullet.velocity, dt));

        bool expired = bullet.lifetime <= 0.0f;
        bool hit = false;
        if (enemy.health > 0) {
            float distSq = Vector3DistanceSqr(bullet.position, enemy.position);
            float radius = enemy.radius + 0.2f;
            if (distSq <= radius * radius) {
                enemy.health -= 25;
                hit = true;
                score += 25;
                if (enemy.health <= 0) {
                    respawnTimer = 1.5f;
                    score += 75;
                }
            }
        }

        if (expired || hit) {
            bullet.lifetime = -1.0f;
        }
    }

    bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const Bullet &b) { return b.lifetime <= 0.0f; }), bullets.end());
}

void GameScene::updateEnemy(float dt) {
    if (enemy.health <= 0) return;

    enemy.patrolT += dt * enemy.patrolSpeed;
    float pathRadius = 6.0f;
    enemy.position.x = std::cos(enemy.patrolT) * pathRadius;
    enemy.position.z = std::sin(enemy.patrolT) * pathRadius + 6.0f;

    // Enemy slowly strafes up/down for motion.
    enemy.position.y = 1.8f + std::sin(timeAlive * 1.5f) * 0.35f;

    // Contact damage (simple proximity check).
    float distToPlayer = Vector3Distance(enemy.position, player.position);
    if (distToPlayer < enemy.radius + 0.8f) {
        player.health = std::max(0, player.health - static_cast<int>(35 * dt));
    }
}

void GameScene::drawWorld() const {
    // Ground
    DrawPlane({0.0f, 0.0f, 6.0f}, {40.0f, 40.0f}, Color{32, 36, 40, 255});
    DrawGrid(20, 2.0f);

    // Enemy target
    if (enemy.health > 0) {
        DrawSphere(enemy.position, enemy.radius + 0.2f, Color{20, 20, 26, 120});
        DrawSphere(enemy.position, enemy.radius, Color{200, 60, 70, 240});
    }

    // Bullets
    for (const auto &bullet : bullets) {
        DrawSphere(bullet.position, 0.12f, Color{255, 230, 160, 255});
    }

    // Simple muzzle flash sphere near camera
    if (muzzleFlashTimer > 0.0f) {
        Vector3 forward = ForwardFromAngles(player.yaw, player.pitch);
        Vector3 flashPos = Vector3Add(player.position, Vector3Scale(forward, 0.6f));
        DrawSphere(flashPos, 0.15f, Color{255, 230, 180, 230});
    }
}

void GameScene::drawHUD() const {
    const int hudWidth = 320;
    DrawRectangle(20, 20, hudWidth, 120, Color{10, 10, 16, 180});
    DrawRectangleLines(20, 20, hudWidth, 120, Color{90, 110, 130, 255});

    int healthBarWidth = 240;
    int healthFill = static_cast<int>(healthBarWidth * (player.health / 100.0f));
    DrawRectangle(40, 60, healthBarWidth, 16, Color{60, 60, 70, 220});
    DrawRectangle(40, 60, healthFill, 16, Color{90, 200, 120, 235});
    DrawText("HEALTH", 40, 40, 14, RAYWHITE);

    std::string scoreText = "Score: " + std::to_string(score);
    DrawText(scoreText.c_str(), 40, 90, 20, RAYWHITE);

    std::string timeText = "Survived: " + std::to_string(static_cast<int>(timeAlive)) + "s";
    DrawText(timeText.c_str(), 40, 114, 14, Color{220, 220, 220, 180});

    // Crosshair
    int cx = config.screenWidth / 2;
    int cy = config.screenHeight / 2;
    DrawLine(cx - 12, cy, cx + 12, cy, Color{230, 230, 230, 240});
    DrawLine(cx, cy - 12, cx, cy + 12, Color{230, 230, 230, 240});

    if (enemy.health <= 0) {
        DrawRectangle(0, 0, config.screenWidth, config.screenHeight, Color{0, 0, 0, 140});
        DrawText("Target down", cx - 80, cy - 10, 24, RAYWHITE);
    }
}

void GameScene::resetEnemy() {
    enemy.health = 80 + score / 3;
    enemy.radius = 0.6f + static_cast<float>(score) * 0.004f;
    enemy.position = {6.0f, 1.8f, 6.0f};
    enemy.patrolT = 0.0f;
    enemy.patrolSpeed = 0.8f + static_cast<float>(score) * 0.002f;
}
