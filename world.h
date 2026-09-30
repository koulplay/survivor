#pragma once

#include <raylib.h>
#include <vector>
#include <array>

constexpr int kScreenWidth = 1280;
constexpr int kScreenHeight = 720;

constexpr float kExpRadius = 2.0f;
constexpr float kExpSpeed = 300.f;

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

constexpr float kDamagePopupSpeed = 10.f;
constexpr float kEnemyFlashTime = 0.05f;

constexpr float kEnemySpawnTimeBase = 2.f;
constexpr float kEnemySpawnTimeRatio = 0.001f;
constexpr float kEnemySpawnTimeMin = 0.5f;

enum class GameState { PLAYING, LEVELUP, GAMEOVER };
enum class Upgrade { FireRate, MoveSpeed, BulletSpeed, Magnet, Health, RestoreHealth, Count };
enum class EnemyType { LIGHT, MEDIUM, HEAVY, COUNT };

[[nodiscard]] Rectangle CardRect(int i);

struct LevelUpMenu {
    std::array<Upgrade, 3> options{};
    int selected = 0;
};

struct Exp {
    Vector2 pos;
    float value;
    bool dead = false;
};

struct Bullet {
    Vector2 pos;
    Vector2 vel;
    float age = 0.f;
    bool dead = false;
};

struct Player {
    float size = 32.f;
    float speed = 150.f;
    float fire_rate = 2.f;
    float bullet_radius = 3.f;
    float bullet_speed = 300.f;
    float bullet_damage = 5.f;

    Vector2 pos{kScreenWidth / 2.f, kScreenHeight / 2.f};
    float health = 100.f;
    float max_health = 100.f;
    float bullet_spawn_time = 0.f;
    float magnet_radius = 60.f;

    [[nodiscard]] Rectangle rect() const {
        return {pos.x - size / 2, pos.y - size / 2, size, size};
    }
};

struct EnemyDef {
    float health;
    float damage;
    float size;
    float speed;
    float exp;
    float time_to_spawn;
    Color color;
};

constexpr EnemyDef kEnemyDefs[] = {
    {.health = 5.f, .damage = 15.f, .size = 5.0f, .speed = 150.f, .exp = 3.f, .time_to_spawn = 30.f, .color = GREEN},
    {.health = 10.f, .damage = 30.f, .size = 10.0f, .speed = 50.f, .exp = 5.f, .time_to_spawn = 0.f, .color = RED},
    {.health = 20.f, .damage = 30.f, .size = 25.0f, .speed = 20.f, .exp = 8.f, .time_to_spawn = 60.f, .color = PURPLE}
};

[[nodiscard]] const EnemyDef& GetEnemyDef(EnemyType type);

struct Enemy {
    Enemy(const Vector2 pos, const EnemyType& enemy_def)
        : pos(pos)
        , enemy_type(enemy_def){
        health = GetEnemyDef(enemy_def).health;
    }

    Vector2 pos;
    EnemyType enemy_type;
    float health;

    float hit_flash_time = 0.f;
    bool dead = false;
};

static_assert(static_cast<std::size_t>(EnemyType::COUNT) == std::size(kEnemyDefs));

struct DamagePopup {
    Vector2 pos;
    Vector2 vel;
    float damage = 0.f;
    float lifetime = 2.f;
    float age = 0.f;
    EnemyType enemy_type;
    bool dead = false;
};

struct World {
    Player player;
    std::vector<Enemy> enemies;
    std::vector<Bullet> bullets;
    std::vector<Exp> exps;
    std::vector<DamagePopup> damage_popups;

    LevelUpMenu lvl_menu;
    GameState game_state = GameState::PLAYING;
    float current_exp = 0.f;
    float exp_to_level = 1.f;
    int level = 0;
    float enemy_spawn_time = 0.f;
    float game_time = 0.f;
};
