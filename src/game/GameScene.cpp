#include "GameScene.h"

#include <raymath.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace {
constexpr float kMaxPitch = 89.0f * DEG2RAD;
constexpr float kMouseSensitivity = 0.0035f;

Vector3 ForwardFromAngles(float yaw, float pitch) {
    float cosPitch = std::cos(pitch);
    return {std::cos(yaw) * cosPitch, std::sin(pitch), std::sin(yaw) * cosPitch};
}
} // namespace

GameScene::GameScene(const GameConfig &config) : config(config) {
    player.position = {0.0f, 1.8f, -6.0f};
    player.camera.position = player.position;
    player.camera.target = Vector3Add(player.position, Vector3{0.0f, 1.6f, 1.0f});
    player.camera.up = {0.0f, 1.0f, 0.0f};
    player.camera.fovy = 75.0f;
    player.camera.projection = CAMERA_PERSPECTIVE;

    loadout = {
        {"SMG", 18.0f, 0.08f, 70.0f, false, 60.0f, false},
        {"LMG", 24.0f, 0.11f, 68.0f, false, 60.0f, false},
        {"Sniper", 95.0f, 1.2f, 140.0f, true, 35.0f, false},
        {"Shotgun", 40.0f, 0.85f, 60.0f, false, 60.0f, false},
        {"Marksman", 55.0f, 0.35f, 90.0f, true, 50.0f, false},
        {"Assault", 32.0f, 0.14f, 75.0f, true, 55.0f, false},
    };
    meleeWeapon = {"Combat Melee", 60.0f, 0.45f, 0.0f, false, 60.0f, true};
    player.fireCooldown = loadout[player.activeWeapon].fireCooldown;

    resetEnemy();
}

void GameScene::update(float dt, const InputState &input) {
    timeAlive += dt;
    muzzleFlashTimer = std::max(0.0f, muzzleFlashTimer - dt);
    respawnTimer = std::max(0.0f, respawnTimer - dt);
    player.fireTimer = std::max(0.0f, player.fireTimer - dt);
    player.slideCooldownTimer = std::max(0.0f, player.slideCooldownTimer - dt);

    handleWeapons(input);
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

    bool wantsToSlide = input.slide && player.slideCooldownTimer <= 0.0f && Vector3Length(move) > 0.15f;
    if (wantsToSlide) {
        player.sliding = true;
        player.slideTimer = 0.65f;
        player.slideCooldownTimer = player.slideCooldown;
        player.slideDirection = Vector3Normalize(move);
    }

    if (player.sliding) {
        player.slideTimer = std::max(0.0f, player.slideTimer - dt);
        Vector3 slideMove = Vector3Scale(player.slideDirection, player.slideSpeed * dt);
        player.position = Vector3Add(player.position, slideMove);
        if (player.slideTimer <= 0.0f) {
            player.sliding = false;
        }
    } else if (Vector3Length(move) > 0.001f) {
        move = Vector3Normalize(move);
        float speed = player.speed;
        if (input.aim) speed *= 0.65f;
        player.position = Vector3Add(player.position, Vector3Scale(move, speed * dt));
    }

    player.height = player.sliding ? 1.25f : 1.8f;

    // Keep player above ground plane.
    player.position.y = player.height;

    player.camera.position = player.position;
    player.camera.target = Vector3Add(player.position, forward);

    applyAiming(dt, input);

    if (input.fire && player.fireTimer <= 0.0f) {
        player.fireTimer = player.fireCooldown;

        if (player.meleeActive) {
            float distToEnemy = Vector3Distance(player.position, enemy.position);
            if (enemy.health > 0 && distToEnemy < 2.4f) {
                enemy.health -= static_cast<int>(meleeWeapon.damage);
                score += 15;
                enemy.currentIntent = "Staggered by melee";
                if (enemy.health <= 0) {
                    respawnTimer = 1.5f;
                    score += 50;
                }
            }
        } else {
            const Weapon &weapon = loadout[player.activeWeapon];
            Bullet bullet{};
            bullet.position = Vector3Add(player.position, {0.0f, -0.1f, 0.0f});
            bullet.velocity = Vector3Scale(forward, weapon.bulletSpeed);
            bullet.lifetime = 2.5f;
            bullet.damage = weapon.damage;
            bullets.push_back(bullet);

            muzzleFlashTimer = 0.08f;
        }
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
                enemy.health -= static_cast<int>(bullet.damage);
                hit = true;
                score += 25;
                enemy.currentIntent = "Hit by projectile";
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

    enemy.thinkTimer += dt;
    enemy.slideCooldownTimer = std::max(0.0f, enemy.slideCooldownTimer - dt);

    Vector3 toPlayer = Vector3Subtract(player.position, enemy.position);
    float distToPlayer = Vector3Length(toPlayer);
    Vector3 dirToPlayer = distToPlayer > 0.001f ? Vector3Scale(toPlayer, 1.0f / distToPlayer) : Vector3{0.0f, 0.0f, 1.0f};

    bool playerThreatening = playerFacingEnemy() && distToPlayer < 16.0f;
    bool closeRange = distToPlayer < 6.0f;

    if (enemy.thinkTimer > 0.35f) {
        enemy.thinkTimer = 0.0f;
        if (!enemy.sliding && enemy.slideCooldownTimer <= 0.0f && (closeRange || playerThreatening)) {
            enemy.sliding = true;
            enemy.slideTimer = 0.65f;
            enemy.slideCooldownTimer = enemy.slideCooldown;
            Vector3 lateral = Vector3CrossProduct(dirToPlayer, {0.0f, 1.0f, 0.0f});
            if (Vector3Length(lateral) > 0.001f) lateral = Vector3Normalize(lateral);
            float directionSign = (static_cast<int>(std::round(timeAlive)) % 2 == 0) ? 1.0f : -1.0f;
            enemy.slideDirection = Vector3Add(Vector3Scale(lateral, directionSign), Vector3Scale(dirToPlayer, -0.25f));
            enemy.slideDirection = Vector3Normalize(enemy.slideDirection);
            enemy.currentIntent = "Dodging with a slide";
        } else if (playerThreatening) {
            enemy.currentIntent = "Flanking";
        } else {
            enemy.currentIntent = "Patrolling";
        }
    }

    if (enemy.sliding) {
        enemy.slideTimer = std::max(0.0f, enemy.slideTimer - dt);
        enemy.position = Vector3Add(enemy.position, Vector3Scale(enemy.slideDirection, 10.0f * dt));
        if (enemy.slideTimer <= 0.0f) {
            enemy.sliding = false;
        }
    } else {
        enemy.patrolT += dt * enemy.patrolSpeed;
        float pathRadius = 6.0f;
        enemy.position.x = std::cos(enemy.patrolT) * pathRadius;
        enemy.position.z = std::sin(enemy.patrolT) * pathRadius + 6.0f;

        // Strafe in/out based on threat assessment.
        if (playerThreatening) {
            enemy.position = Vector3Add(enemy.position, Vector3Scale(dirToPlayer, -1.2f * dt));
        } else if (closeRange) {
            enemy.position = Vector3Add(enemy.position, Vector3Scale(dirToPlayer, -0.4f * dt));
        } else {
            enemy.position = Vector3Add(enemy.position, Vector3Scale(dirToPlayer, 0.6f * dt));
        }
    }

    // Enemy slowly strafes up/down for motion.
    enemy.position.y = 1.8f + std::sin(timeAlive * 1.5f) * 0.35f;

    // Contact damage (simple proximity check).
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
    const int hudWidth = 360;
    DrawRectangle(20, 20, hudWidth, 190, Color{10, 10, 16, 180});
    DrawRectangleLines(20, 20, hudWidth, 190, Color{90, 110, 130, 255});

    int healthBarWidth = 240;
    int healthFill = static_cast<int>(healthBarWidth * (player.health / 100.0f));
    DrawRectangle(40, 60, healthBarWidth, 16, Color{60, 60, 70, 220});
    DrawRectangle(40, 60, healthFill, 16, Color{90, 200, 120, 235});
    DrawText("HEALTH", 40, 40, 14, RAYWHITE);

    std::string scoreText = "Score: " + std::to_string(score);
    DrawText(scoreText.c_str(), 40, 90, 20, RAYWHITE);

    std::string timeText = "Survived: " + std::to_string(static_cast<int>(timeAlive)) + "s";
    DrawText(timeText.c_str(), 40, 114, 14, Color{220, 220, 220, 180});

    const Weapon &weapon = player.meleeActive ? meleeWeapon : loadout[player.activeWeapon];
    std::string weaponLine = "Weapon: " + weapon.name;
    DrawText(weaponLine.c_str(), 40, 138, 16, RAYWHITE);
    std::string aimHint = "RMB to zoom | CTRL to slide | Q melee";
    DrawText(aimHint.c_str(), 40, 160, 14, Color{200, 200, 200, 180});

    DrawText(("Enemy: " + enemy.currentIntent).c_str(), 40, 182, 14, Color{190, 230, 255, 200});

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
    enemy.sliding = false;
    enemy.slideTimer = 0.0f;
    enemy.slideCooldownTimer = 1.5f;
    enemy.currentIntent = "Respawning";
}

void GameScene::handleWeapons(const InputState &input) {
    if (input.weaponHotkey >= 0 && input.weaponHotkey < static_cast<int>(loadout.size())) {
        player.activeWeapon = input.weaponHotkey;
        player.meleeActive = false;
        const Weapon &weapon = loadout[player.activeWeapon];
        player.fireCooldown = weapon.fireCooldown;
    }

    if (input.toggleMelee) {
        player.meleeActive = !player.meleeActive;
        player.fireCooldown = player.meleeActive ? meleeWeapon.fireCooldown : loadout[player.activeWeapon].fireCooldown;
    }
}

void GameScene::applyAiming(float dt, const InputState &input) {
    player.aiming = input.aim;
    float targetFov = 75.0f;
    const Weapon &weapon = player.meleeActive ? meleeWeapon : loadout[player.activeWeapon];
    if (player.aiming) {
        if (weapon.scoped) targetFov = weapon.scopedFov;
        else targetFov = 60.0f;
    }
    float fov = Lerp(player.camera.fovy, targetFov, std::clamp(dt * 12.0f, 0.0f, 1.0f));
    player.camera.fovy = std::clamp(fov, 30.0f, 90.0f);
}

bool GameScene::playerFacingEnemy() const {
    Vector3 forward = ForwardFromAngles(player.yaw, player.pitch);
    Vector3 toEnemy = Vector3Subtract(enemy.position, player.position);
    if (Vector3Length(toEnemy) < 0.001f) return true;
    toEnemy = Vector3Normalize(toEnemy);
    float alignment = Vector3DotProduct(forward, toEnemy);
    return alignment > 0.75f;
}
