#include "GameScene.h"

#include <raymath.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace {
constexpr float kMaxPitch = 89.0f * DEG2RAD;
constexpr float kMouseSensitivity = 0.0035f;
constexpr float kEnemyAimSmoothing = 6.0f;
constexpr float kOptimalEngageDistance = 10.0f;
constexpr float kMaxEngageDistance = 26.0f;
constexpr float kBulletDamage = 25.0f;

Vector3 ForwardFromAngles(float yaw, float pitch) {
    float cosPitch = std::cos(pitch);
    return {std::cos(yaw) * cosPitch, std::sin(pitch), std::sin(yaw) * cosPitch};
}

float Approach(float current, float target, float delta) {
    if (current < target) return std::min(target, current + delta);
    return std::max(target, current - delta);
}

float NormalizeAngle(float angle) {
    while (angle > PI) angle -= 2.0f * PI;
    while (angle < -PI) angle += 2.0f * PI;
    return angle;
}

float MoveTowardsAngle(float current, float target, float maxDelta) {
    float delta = NormalizeAngle(target - current);
    delta = std::clamp(delta, -maxDelta, maxDelta);
    return NormalizeAngle(current + delta);
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
    enemyMuzzleFlashTimer = std::max(0.0f, enemyMuzzleFlashTimer - dt);
    respawnTimer = std::max(0.0f, respawnTimer - dt);
    player.fireTimer = std::max(0.0f, player.fireTimer - dt);
    enemy.fireTimer = std::max(0.0f, enemy.fireTimer - dt);

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
    Vector3 prevPos = player.position;
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

    if (dt > 0.0f) {
        player.velocity = Vector3Scale(Vector3Subtract(player.position, prevPos), 1.0f / dt);
    }

    if (input.fire && player.fireTimer <= 0.0f) {
        Bullet bullet{};
        bullet.position = Vector3Add(player.position, {0.0f, -0.1f, 0.0f});
        bullet.velocity = Vector3Scale(forward, 65.0f);
        bullet.lifetime = 2.5f;
        bullet.fromPlayer = true;
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

        if (bullet.fromPlayer && enemy.health > 0) {
            float distSq = Vector3DistanceSqr(bullet.position, enemy.position);
            float radius = enemy.radius + 0.2f;
            if (distSq <= radius * radius) {
                enemy.health -= static_cast<int>(kBulletDamage);
                hit = true;
                score += 25;
                if (enemy.health <= 0) {
                    respawnTimer = 1.5f;
                    score += 75;
                }
            }
        } else if (!bullet.fromPlayer && player.health > 0) {
            float distSq = Vector3DistanceSqr(bullet.position, player.position);
            float radius = 0.7f;
            if (distSq <= radius * radius) {
                player.health = std::max(0, player.health - static_cast<int>(kBulletDamage));
                hit = true;
            }
        }

        if (expired || hit) bullet.lifetime = -1.0f;
    }

    bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const Bullet &b) { return b.lifetime <= 0.0f; }), bullets.end());
}

void GameScene::updateEnemy(float dt) {
    if (enemy.health <= 0) return;

    // Strategic movement: maintain distance, strafe unpredictably, and reposition when too close/far.
    enemy.strafeTimer -= dt;
    if (enemy.strafeTimer <= 0.0f) {
        enemy.strafeDir = (std::sin(timeAlive * 1.7f) > 0.0f) ? 1.0f : -1.0f;
        enemy.strafeTimer = 1.0f + std::fmod(timeAlive, 0.8f);
    }

    Vector3 prevPos = enemy.position;
    Vector3 toPlayer = Vector3Subtract(player.position, enemy.position);
    float distToPlayer = Vector3Length(toPlayer);
    Vector3 dirToPlayer = (distToPlayer > 0.001f) ? Vector3Scale(toPlayer, 1.0f / distToPlayer) : Vector3{0.0f, 0.0f, 1.0f};

    // Desired movement blends closing/opening distance with lateral strafing to avoid being a static target.
    Vector3 flatDir = {dirToPlayer.x, 0.0f, dirToPlayer.z};
    if (Vector3Length(flatDir) > 0.001f) flatDir = Vector3Normalize(flatDir);
    Vector3 forward = ForwardFromAngles(enemy.yaw, enemy.pitch);
    Vector3 flatForward = {forward.x, 0.0f, forward.z};
    if (Vector3Length(flatForward) > 0.001f) flatForward = Vector3Normalize(flatForward);
    Vector3 right = Vector3CrossProduct(flatForward, {0.0f, 1.0f, 0.0f});
    if (Vector3Length(right) > 0.001f) right = Vector3Normalize(right);

    Vector3 move = {0.0f, 0.0f, 0.0f};
    if (distToPlayer > kOptimalEngageDistance + 1.5f && distToPlayer < kMaxEngageDistance + 4.0f) {
        move = Vector3Add(move, flatDir); // close distance
    } else if (distToPlayer < kOptimalEngageDistance - 1.0f) {
        move = Vector3Subtract(move, flatDir); // backpedal to keep space
    }
    move = Vector3Add(move, Vector3Scale(right, enemy.strafeDir * 0.8f));

    if (Vector3Length(move) > 0.001f) {
        move = Vector3Normalize(move);
        enemy.position = Vector3Add(enemy.position, Vector3Scale(move, enemy.speed * dt));
    }

    enemy.position.y = enemy.height;
    if (dt > 0.0f) enemy.velocity = Vector3Scale(Vector3Subtract(enemy.position, prevPos), 1.0f / dt);

    // Predictive aiming to account for player velocity.
    Vector3 predictedPlayerPos = Vector3Add(player.position, Vector3Scale(player.velocity, 0.25f));
    Vector3 aimVector = Vector3Subtract(predictedPlayerPos, enemy.position);
    float aimLength = Vector3Length(aimVector);
    Vector3 aimDir = (aimLength > 0.001f) ? Vector3Scale(aimVector, 1.0f / aimLength) : Vector3{0.0f, 0.0f, 1.0f};
    float desiredYaw = std::atan2(aimDir.z, aimDir.x);
    float desiredPitch = std::asin(std::clamp(aimDir.y, -1.0f, 1.0f));

    enemy.yaw = MoveTowardsAngle(enemy.yaw, desiredYaw, kEnemyAimSmoothing * dt);
    enemy.pitch = std::clamp(Approach(enemy.pitch, desiredPitch, kEnemyAimSmoothing * dt), -kMaxPitch, kMaxPitch);

    // Fire only when within range and roughly on target.
    forward = ForwardFromAngles(enemy.yaw, enemy.pitch);
    float forwardLen = Vector3Length(forward);
    if (forwardLen > 0.001f) forward = Vector3Scale(forward, 1.0f / forwardLen);
    float alignment = Vector3DotProduct(forward, aimDir);
    bool inRange = distToPlayer < kMaxEngageDistance;
    bool hasAim = alignment > std::cos(10.0f * DEG2RAD);
    if (inRange && hasAim && enemy.fireTimer <= 0.0f && player.health > 0) {
        Bullet bullet{};
        bullet.fromPlayer = false;
        bullet.position = Vector3Add(enemy.position, {0.0f, -0.1f, 0.0f});
        bullet.velocity = Vector3Scale(forward, 65.0f);
        bullet.lifetime = 2.5f;
        bullets.push_back(bullet);

        enemyMuzzleFlashTimer = 0.08f;
        enemy.fireTimer = enemy.fireCooldown;
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

    if (enemyMuzzleFlashTimer > 0.0f && enemy.health > 0) {
        Vector3 forward = ForwardFromAngles(enemy.yaw, enemy.pitch);
        Vector3 flashPos = Vector3Add(enemy.position, Vector3Scale(forward, 0.6f));
        DrawSphere(flashPos, 0.12f, Color{255, 170, 140, 200});
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
    enemy.fireTimer = 0.4f;
    enemy.yaw = std::atan2(player.position.z - enemy.position.z, player.position.x - enemy.position.x);
    enemy.pitch = 0.0f;
    enemy.strafeTimer = 0.0f;
    enemy.velocity = {0.0f, 0.0f, 0.0f};
}
