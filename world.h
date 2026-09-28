#pragma once

#include <raylib.h>
#include <vector>
#include <array>

constexpr int kScreenWidth = 1280;
constexpr int kScreenHeight = 720;

constexpr float kEnemyRadius = 10.0f;
constexpr float kEnemySpeed = 50.f;
constexpr float kEnemyDamage = 30.f;

constexpr float kExpRadius = 2.0f;
constexpr float kExpSpeed = 300.f;
constexpr float kExpValue = 5.f;

constexpr float kExpBarWidth = 300.f;
constexpr float kExpBarHeight = 50.f;
constexpr Vector2 kExpBarPos = {kScreenWidth - 320.f, 20.f};

constexpr float kCardWidth = 300.f;
constexpr float kCardHeight = 500.f;
constexpr float kCardGap = 50.f;

constexpr float kBuffFireRate = 0.2f;
constexpr float kBuffMoveSpeed = 10.f;
constexpr float kBuffBulletSpeed = 25.f;
constexpr float kBuffMagnet = 5.f;
constexpr float kBuffHealth = 5.f;
constexpr float kBuffRestoreHealth = 25.f;

constexpr float kPlayerBulletMaxLifetime = 10.f;

enum class GameState {
    PLAYING,
    LEVELUP,
    GAMEOVER
};

enum class Upgrade { FireRate, MoveSpeed, BulletSpeed, Magnet, Health, RestoreHealth, Count };

struct LevelUpMenu {
    std::array<Upgrade, 3> options{};
    int selected = 0;
};

struct Exp {
    Vector2 pos;
    bool dead = false;
};

struct Bullet {
    Vector2 pos;
    Vector2 vel;
    float lifetime = 0.f;
    bool dead = false;
};

struct Player {
    float size = 32.0f;
    float speed = 150.f;
    float fire_rate = 2.f;
    float bullet_radius = 3.0f;
    float bullet_speed = 300.f;

    Vector2 pos{kScreenWidth / 2.f, kScreenHeight / 2.f};
    float health = 100.f;
    float max_health = 100.f;
    float bullet_spawn_time = 0.f;
    float magnet_radius = 60.f;

    [[nodiscard]] Rectangle rect() const {
        return {pos.x - size / 2, pos.y - size / 2, size, size};
    }
};

struct Enemy {
    Vector2 pos;
    bool dead = false;
};

struct World {
    Player player;
    std::vector<Enemy> enemies;
    std::vector<Bullet> bullets;
    std::vector<Exp> exps;

    LevelUpMenu lvl_menu;
    GameState game_state = GameState::PLAYING;
    float current_exp = 0.f;
    float exp_to_level = 1.f;
    int level = 0;
    float enemy_spawn_time = 0.f;
    float game_time = 0.f;
};

[[nodiscard]] Rectangle CardRect(int i);

