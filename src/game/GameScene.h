#pragma once

#include "engine/Config.h"
#include "engine/Input.h"
#include "engine/Scene.h"

#include <raylib.h>

#include <string>
#include <vector>

struct Bullet {
    Vector3 position{};
    Vector3 velocity{};
    float damage = 25.0f;
    float lifetime = 2.0f;
};

struct Weapon {
    std::string name;
    float damage = 25.0f;
    float fireCooldown = 0.12f;
    float bulletSpeed = 65.0f;
    bool scoped = false;
    float scopedFov = 55.0f;
    bool isMelee = false;
};

struct Player {
    Vector3 position{0.0f, 1.8f, 0.0f};
    float yaw = 0.0f;
    float pitch = 0.0f;
    float speed = 6.0f;
    float slideSpeed = 14.0f;
    float height = 1.8f;
    float fireCooldown = 0.12f;
    float fireTimer = 0.0f;
    int health = 100;
    bool sliding = false;
    float slideTimer = 0.0f;
    float slideCooldown = 1.0f;
    float slideCooldownTimer = 0.0f;
    Vector3 slideDirection{};
    bool aiming = false;
    bool meleeActive = false;
    int activeWeapon = 0;
    Camera3D camera{};
};

struct Enemy {
    Vector3 position{0.0f, 1.8f, 8.0f};
    float radius = 0.8f;
    int health = 100;
    float patrolT = 0.0f;
    float patrolSpeed = 1.0f;
    bool sliding = false;
    float slideTimer = 0.0f;
    float slideCooldown = 2.6f;
    float slideCooldownTimer = 0.0f;
    Vector3 slideDirection{};
    float thinkTimer = 0.0f;
    std::string currentIntent = "Patrolling";
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
    void handleWeapons(const InputState &input);
    void applyAiming(float dt, const InputState &input);
    bool playerFacingEnemy() const;

    GameConfig config;
    Player player{};
    Enemy enemy{};
    std::vector<Weapon> loadout;
    Weapon meleeWeapon{};
    std::vector<Bullet> bullets;
    int score = 0;
    float respawnTimer = 0.0f;
    float timeAlive = 0.0f;
    float muzzleFlashTimer = 0.0f;
};
