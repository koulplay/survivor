#pragma once

#include <raylib.h>
#include <vector>
#include <array>

constexpr int kScreenWidth = 1280;
constexpr int kScreenHeight = 720;

constexpr float kExpRadius = 2.0f;
constexpr float kExpSpeed = 300.f;
constexpr float kExpToLevelMultiplier = 1.2f;

constexpr float kExpBarWidth = 300.f;
constexpr float kExpBarHeight = 50.f;
constexpr Vector2 kExpBarPos = {kScreenWidth - 320.f, 20.f};

constexpr float kCardWidth = 300.f;
constexpr float kCardHeight = 500.f;
constexpr float kCardGap = 50.f;

constexpr float kUpgradeFireInterval = 0.2f;
constexpr float kUpgradeMoveSpeed = 10.f;
constexpr float kUpgradeBulletSpeed = 25.f;
constexpr float kUpgradeMagnet = 5.f;
constexpr float kUpgradeMaxHealth = 5.f;
constexpr float kUpgradeRestoreHealth = 25.f;

constexpr float kPlayerBulletMaxLifetime = 10.f;

constexpr float kDamagePopupSpeed = 10.f;
constexpr float kEnemyFlashDuration = 0.05f;

constexpr float kEnemySpawnIntervalStart = 2.f;
constexpr float kEnemySpawnIntervalDecreasePerSec = 0.001f;
constexpr float kEnemySpawnIntervalMin = 0.5f;

constexpr float kPlayerInvulnerabilityDuration = 0.6f;
constexpr float kPlayerBlinkPhaseDuration = 0.2f;

constexpr float kSeparationSpeedFactor = 5.f;

enum class GameState { Playing, LevelUp, GameOver };

enum class Upgrade { FireInterval, MoveSpeed, BulletSpeed, Magnet, MaxHealth, RestoreHealth, Count };

enum class EnemyType { Light, Medium, Heavy, Splitter, Twin, Count };

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
    float fire_interval = 2.f;
    float bullet_radius = 3.f;
    float bullet_speed = 300.f;
    float bullet_damage = 5.f;

    Vector2 pos{kScreenWidth / 2.f, kScreenHeight / 2.f};
    float health = 100.f;
    float max_health = 100.f;
    float shot_timer = 0.f;
    float magnet_radius = 60.f;

    float invulnerability_timer = 0.f;

    [[nodiscard]] Rectangle rect() const {
        return {pos.x - size / 2, pos.y - size / 2, size, size};
    }
};

struct EnemyDef {
    float max_health;
    float damage;
    float radius;
    float speed;
    float exp;
    float appear_time;
    Color color;
};

constexpr EnemyDef kEnemyDefs[] = {
    { .max_health = 5.f,  .damage = 15.f, .radius = 5.f,  .speed = 150.f, .exp = 3.f, .appear_time = 30.f, .color = GREEN },       //Light
    { .max_health = 10.f, .damage = 30.f, .radius = 10.f, .speed = 50.f,  .exp = 5.f, .appear_time = 0.f,  .color = RED },         //Medium
    { .max_health = 20.f, .damage = 30.f, .radius = 25.f, .speed = 20.f,  .exp = 8.f, .appear_time = 60.f, .color = PURPLE },      //Heavy
    { .max_health = 8.f,  .damage = 20.f, .radius = 10.f, .speed = 50.f,  .exp = 0.f, .appear_time = 30.f, .color = GRAY },        //Splitter
    { .max_health = 3.f,  .damage = 30.f, .radius = 8.f,  .speed = 20.f,  .exp = 2.f, .appear_time = -1.f, .color = LIGHTGRAY }    //Twin
};

[[nodiscard]] const EnemyDef &GetEnemyDef(EnemyType type);

struct Enemy {
    Enemy(const Vector2 pos, const EnemyType &type)
        : pos(pos)
          , type(type) {
        health = GetEnemyDef(type).max_health;
    }

    Vector2 pos;
    EnemyType type;
    float health;

    float hit_flash_time = 0.f;
    bool dead = false;
};

static_assert(static_cast<std::size_t>(EnemyType::Count) == std::size(kEnemyDefs));

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

    LevelUpMenu level_up_menu;
    GameState game_state = GameState::Playing;
    float current_exp = 0.f;
    float exp_to_level = 1.f;
    int level = 0;
    float enemy_spawn_timer = 0.f;
    float game_time = 0.f;
};
