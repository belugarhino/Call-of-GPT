#pragma once

#include "engine/Config.h"
#include "engine/Input.h"
#include "engine/Scene.h"

#include <raylib.h>

#include <vector>

struct Bullet {
    Vector3 position{};
    Vector3 velocity{};
    float lifetime = 2.0f;
    bool fromPlayer = true;
};

struct Player {
    Vector3 position{0.0f, 1.8f, 0.0f};
    Vector3 velocity{};
    float yaw = 0.0f;
    float pitch = 0.0f;
    float speed = 6.0f;
    float height = 1.8f;
    float fireCooldown = 0.12f;
    float fireTimer = 0.0f;
    int health = 100;
    Camera3D camera{};
};

struct Enemy {
    Vector3 position{0.0f, 1.8f, 8.0f};
    Vector3 velocity{};
    float yaw = 0.0f;
    float pitch = 0.0f;
    float speed = 6.0f;
    float height = 1.8f;
    float fireCooldown = 0.12f;
    float fireTimer = 0.0f;
    float radius = 0.8f;
    int health = 100;
    float patrolT = 0.0f;
    float patrolSpeed = 1.0f;
    float strafeDir = 1.0f;
    float strafeTimer = 0.0f;
};

class GameScene : public Scene {
  public:
    explicit GameScene(const GameConfig &config);

    void update(float dt, const InputState &input) override;
    void fixedUpdate(float dt, const InputState &input) override;
    void render() override;

  private:
    void updatePlayer(float dt, const InputState &input);
    void updateBullets(float dt);
    void updateEnemy(float dt);
    void drawHUD() const;
    void drawWorld() const;
    void resetEnemy();

    GameConfig config;
    Player player{};
    Enemy enemy{};
    std::vector<Bullet> bullets;
    int score = 0;
    float respawnTimer = 0.0f;
    float timeAlive = 0.0f;
    float muzzleFlashTimer = 0.0f;
    float enemyMuzzleFlashTimer = 0.0f;
};
