#include "update.h"

#include <algorithm>
#include <array>
#include <optional>
#include <random>
#include <string>

#include "raylib.h"
#include "raymath.h"
#include "world.h"

namespace {
[[nodiscard]] Vector2 RandomEdgePoint(const Player &player) {
    const auto x = static_cast<float>(GetRandomValue(static_cast<int>(player.pos.x) - kScreenWidth / 2,
                                                     static_cast<int>(player.pos.x) + kScreenWidth / 2));
    const auto y = static_cast<float>(GetRandomValue(static_cast<int>(player.pos.y) - kScreenHeight / 2,
                                                     static_cast<int>(player.pos.y) + kScreenHeight / 2));
    switch (GetRandomValue(0, 3)) {
        case 0: return {x, player.pos.y - kScreenHeight / 2};
        case 1: return {x, player.pos.y + kScreenHeight / 2};
        case 2: return {player.pos.x - kScreenWidth / 2, y};
        default: return {player.pos.x + kScreenWidth / 2, y};
    }
}

[[nodiscard]] Vector2 ReadMoveInput() {
    Vector2 move_direction{0.0f, 0.0f};

    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) {
        move_direction += {0.0f, -1.f};
    }
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
        move_direction += {-1.f, 0.0f};
    }
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) {
        move_direction += {0.0f, 1.f};
    }
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
        move_direction += {1.f, 0.0f};
    }
    return Vector2Normalize(move_direction);
}

[[nodiscard]] std::optional<Vector2> NearestEnemyPos(const World &world) {
    if (world.enemies.empty()) {
        return std::nullopt;
    }

    float distance_to_enemy = Vector2Distance(world.player.pos, world.enemies[0].pos);
    Vector2 enemy_pos = world.enemies[0].pos;
    for (auto &e: world.enemies) {
        const float new_distance = Vector2Distance(world.player.pos, e.pos);
        if (new_distance < distance_to_enemy) {
            distance_to_enemy = new_distance;
            enemy_pos = e.pos;
        }
    }
    return enemy_pos;
}


void UpdateSpawn(World &world, const float dt) {
    world.enemy_spawn_time += dt;
    if (world.enemy_spawn_time > 1.0f) {
        world.enemy_spawn_time -= 1.0f;
        world.enemies.emplace_back(RandomEdgePoint(world.player));
    }
}

void UpdatePlayer(Player &player, const float dt) {
    player.pos += ReadMoveInput() * (player.speed * dt);
}

void UpdateEnemies(World &world, const float dt) {
    for (auto &e: world.enemies) {
        e.hit_flash_time = std::max(0.f, e.hit_flash_time - dt);
        e.pos = Vector2MoveTowards(e.pos, world.player.pos, kEnemySpeed * dt);
        if (CheckCollisionCircleRec(e.pos, kEnemyRadius, world.player.rect())) {
            world.player.health -= kEnemyDamage * dt;
        }
    }
}

// таймер + новая пуля в ближайшего
void UpdateShooting(World &world, const float dt) {
    Player &p = world.player;
    p.bullet_spawn_time += dt;
    if (p.bullet_spawn_time < p.fire_rate) return;

    const std::optional<Vector2> target = NearestEnemyPos(world);
    if (!target) {
        p.bullet_spawn_time = p.fire_rate;
        return;
    }

    p.bullet_spawn_time -= p.fire_rate;
    world.bullets.push_back({p.pos, Vector2Normalize(*target - p.pos) * p.bullet_speed});
}

[[nodiscard]] bool ShouldDieByLifetime(const Bullet &b) {
    return b.age >= kPlayerBulletMaxLifetime;
}

// полёт, попадания, удаление
void UpdateBullets(World &world, const float dt) {
    for (auto &b: world.bullets) {
        if (ShouldDieByLifetime(b)) {
            b.dead = true;
            continue;
        }
        if (b.dead) {
            continue;
        }
        b.age += dt;
        for (auto &e: world.enemies) {
            if (e.dead) {
                continue;
            }
            if (b.dead) {
                break;
            }
            if (CheckCollisionCircles(b.pos, world.player.bullet_radius, e.pos, kEnemyRadius)) {
                e.hit_flash_time = kEnemyFlashTime;
                world.damage_popups.push_back({
                    .pos = e.pos,
                    .vel = Vector2{0.f, -kDamagePopupSpeed},
                    .damage = world.player.bullet_damage
                });
                e.health -= world.player.bullet_damage;
                if (e.health <= 0.f) {
                    e.dead = true;
                    b.dead = true;
                    world.exps.push_back({e.pos});
                    break;
                }
                b.dead = true;
            }
        }
        b.pos += b.vel * dt;
    }
}

void UpdateExp(World &world, const float dt) {
    for (auto &exp: world.exps) {
        if (CheckCollisionCircles(world.player.pos, world.player.magnet_radius, exp.pos, kExpRadius)) {
            exp.pos = Vector2MoveTowards(exp.pos, world.player.pos, kExpSpeed * dt);
        }
        if (CheckCollisionCircleRec(exp.pos, kExpRadius, world.player.rect())) {
            world.current_exp += kExpValue;
            exp.dead = true;
        }
    }
}

void UpdateDamagePopup(World &world, const float dt) {
    for (auto &dp: world.damage_popups) {
        dp.pos += dp.vel * dt;
        dp.age += dt;
        if (dp.age >= dp.lifetime) {
            dp.dead = true;
        }
    }
}

void UpdateGameState(World &world) {
    if (world.player.health <= 0.f && world.game_state != GameState::GAMEOVER) {
        world.player.health = 0.f;
        world.game_state = GameState::GAMEOVER;
    }
    else if (world.current_exp >= world.exp_to_level && world.game_state != GameState::LEVELUP) {
        static std::mt19937 rng{std::random_device{}()};
        std::array<Upgrade, static_cast<int>(Upgrade::Count)> all{};
        for (int i = 0; i < static_cast<int>(all.size()); ++i) all[i] = static_cast<Upgrade>(i);
        std::shuffle(all.begin(), all.end(), rng);
        for (int i = 0; i < 3; ++i) world.lvl_menu.options[i] = all[i];
        world.lvl_menu.selected = 0;

        ++world.level;
        const float mod = world.current_exp - world.exp_to_level;
        world.current_exp = mod;
        world.exp_to_level *= 1.2f;
        world.game_state = GameState::LEVELUP;
    }
    else {
        world.game_state = GameState::PLAYING;
    }
}
}


void Update(World &world, const float dt) {
    world.game_time += dt;
    UpdateSpawn(world, dt);
    UpdatePlayer(world.player, dt);
    UpdateEnemies(world, dt);
    UpdateShooting(world, dt);
    UpdateBullets(world, dt);
    UpdateExp(world, dt);
    UpdateDamagePopup(world, dt);

    UpdateGameState(world);

    std::erase_if(world.enemies, [](const Enemy &e) { return e.dead; });
    std::erase_if(world.bullets, [](const Bullet &b) { return b.dead; });
    std::erase_if(world.exps, [](const Exp &e) { return e.dead; });
    std::erase_if(world.damage_popups, [](const DamagePopup &dp) { return dp.dead; });
}

void ApplyUpgrade(Player &p, Upgrade u) {
    // только изменение
    switch (u) {
        case Upgrade::FireRate: p.fire_rate = std::max(0.1f, p.fire_rate - kBuffFireRate);
            break;
        case Upgrade::MoveSpeed: p.speed += kBuffMoveSpeed;
            break;
        case Upgrade::BulletSpeed: p.bullet_speed += kBuffBulletSpeed;
            break;
        case Upgrade::Magnet: p.magnet_radius += kBuffMagnet;
            break;
        case Upgrade::Health: p.max_health += kBuffHealth;
            p.health += kBuffHealth;
            break;
        case Upgrade::RestoreHealth: p.health = std::min(p.health + kBuffRestoreHealth, p.max_health);
            break;
        case Upgrade::Count: break;
    }
}

bool UpdateLevelUp(World &world) {
    auto &m = world.lvl_menu;
    if (IsKeyPressed(KEY_RIGHT)) m.selected = (m.selected + 1) % 3;
    if (IsKeyPressed(KEY_LEFT)) m.selected = (m.selected + 3 - 1) % 3;
    const bool mouse_moved = Vector2Length(GetMouseDelta()) > 0.0f;
    bool confirm = IsKeyPressed(KEY_ENTER);
    for (int i = 0; i < 3; ++i) {
        if (IsKeyPressed(KEY_ONE + i)) {
            m.selected = i;
            confirm = true;
        }
        if (CheckCollisionPointRec(GetMousePosition(), CardRect(i))) {
            if (mouse_moved) m.selected = i;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) confirm = true;
        }
    }
    return confirm;
}
