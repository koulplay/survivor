#include "update.h"

#include <raylib.h>
#include <raymath.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <optional>
#include <random>
#include <string>

#include "world.h"

namespace {
[[nodiscard]] Vector2 RandomEdgePoint(World &world, const float margin) {
    const Vector2 player_pos = world.player.pos;
    std::uniform_int_distribution<> dist_x(static_cast<int>(player_pos.x) - kScreenWidth / 2,
                                           static_cast<int>(player_pos.x) + kScreenWidth / 2);
    std::uniform_int_distribution<> dist_y(static_cast<int>(player_pos.y) - kScreenHeight / 2,
                                           static_cast<int>(player_pos.y) + kScreenHeight / 2);

    const float x = static_cast<float>(dist_x(world.rng));
    const float y = static_cast<float>(dist_y(world.rng));

    // Выбор одной из сторон экрана
    switch (std::uniform_int_distribution<>(0,3)(world.rng)) {
        case 0: return {x, player_pos.y - kScreenHeight / 2 - margin};
        case 1: return {x, player_pos.y + kScreenHeight / 2 + margin};
        case 2: return {player_pos.x - kScreenWidth / 2 - margin, y};
        default: return {player_pos.x + kScreenWidth / 2 + margin, y};
    }
}

[[nodiscard]] Vector2 MovePlayer(const Input& input) {
    Vector2 move_direction{0.0f, 0.0f};
    if (input.down.player_move_up) {
        move_direction += {0.0f, -1.f};
    }
    if (input.down.player_move_left) {
        move_direction += {-1.f, 0.0f};
    }
    if (input.down.player_move_down) {
        move_direction += {0.0f, 1.f};
    }
    if (input.down.player_move_right) {
        move_direction += {1.f, 0.0f};
    }
    return Vector2Normalize(move_direction);
}

[[nodiscard]] std::optional<Vector2> NearestEnemyPos(const World &world) {
    if (world.enemies.empty()) {
        return std::nullopt;
    }

    float best_distance = Vector2Distance(world.player.pos, world.enemies[0].pos);
    Vector2 nearest_pos = world.enemies[0].pos;
    for (auto &e: world.enemies) {
        const float new_distance = Vector2Distance(world.player.pos, e.pos);
        if (new_distance < best_distance) {
            best_distance = new_distance;
            nearest_pos = e.pos;
        }
    }
    return nearest_pos;
}

[[nodiscard]] float EnemySpawnInterval(const float game_time) {
    return std::max(kEnemySpawnIntervalMin, kEnemySpawnIntervalStart - game_time * kEnemySpawnIntervalDecreasePerSec);
}

[[nodiscard]] EnemyType RandomEnemyType(World &world) {
    std::vector<EnemyType> candidates;
    candidates.reserve(static_cast<size_t>(EnemyType::Count));
    for (size_t i = 0; i < static_cast<size_t>(EnemyType::Count); ++i) {
        if (const float app_t = GetEnemyDef(static_cast<EnemyType>(i)).appear_time; app_t <= world.game_time && app_t >= 0) {
            candidates.push_back(static_cast<EnemyType>(i));
        }
    }
    assert(!candidates.empty());
    return candidates[std::uniform_int_distribution<>(0,candidates.size() - 1)(world.rng)];
}

void SpawnEnemy(World &world) {
    const EnemyType enemy_type = RandomEnemyType(world);
    world.enemies.emplace_back(RandomEdgePoint(world, GetEnemyDef(enemy_type).radius), enemy_type);
}

void UpdateSpawn(World &world, const float dt) {
    world.enemy_spawn_timer += dt;
    if (const float interval = EnemySpawnInterval(static_cast<float>(world.game_time)); world.enemy_spawn_timer > interval) {
        world.enemy_spawn_timer -= interval;
        SpawnEnemy(world);
    }
}

void UpdatePlayer(Player &player, const Input& input, const float dt) {
    if (player.invulnerability_timer > 0.f) {
        player.invulnerability_timer = std::max(0.f, player.invulnerability_timer - dt);
    }
    player.pos += MovePlayer(input) * (player.speed * dt);
}

void UpdateEnemies(World &world, const float dt) {
    for (auto &e: world.enemies) {
        const EnemyDef &e_def = GetEnemyDef(e.type);
        e.hit_flash_time = std::max(0.f, e.hit_flash_time - dt);
        e.pos = Vector2MoveTowards(e.pos, world.player.pos, e_def.speed * dt);
    }
}

void UpdateSeparation(World &world, const float dt) {
    for (auto &self: world.enemies) {
        const auto &self_def = GetEnemyDef(self.type);
        for (auto &other: world.enemies) {
            if (&self == &other) continue;
            const auto &other_def = GetEnemyDef(other.type);
            if (!CheckCollisionCircles(self.pos, self_def.radius, other.pos, other_def.radius)) continue;

            const float r = Vector2Length(self.pos - other.pos);
            const float weight = 1.f - r / (self_def.radius + other_def.radius);
            Vector2 direction = Vector2Normalize(other.pos - self.pos);
            if (r < 0.0001f) {
                if (&self > &other) direction = {1.f, 0.f};
                else direction = {-1.f, 0.f};
            }
            Vector2 push = direction * weight * other_def.speed * kSeparationSpeedFactor * dt;
            push = Vector2ClampValue(push, 0.f,other_def.radius);
            other.pos += push;
        }
    }
}

[[nodiscard]] std::optional<float> MaxContactDamage(const World &world) {
    std::optional<float> max_damage;
    for (auto &e: world.enemies) {
        const EnemyDef &e_def = GetEnemyDef(e.type);
        if (CheckCollisionCircleRec(e.pos, e_def.radius, world.player.rect())) {
            if (max_damage != std::nullopt) {
                max_damage = std::max(max_damage.value(), e_def.damage);
            }
            else {
                max_damage = e_def.damage;
            }
        }
    }
    return max_damage;
}

void UpdateContactDamage(World &world) {
    if (world.player.invulnerability_timer > 0.f) return;
    if (const std::optional max_damage = MaxContactDamage(world); max_damage != std::nullopt) {
        world.player.health -= max_damage.value();
        world.player.invulnerability_timer = kPlayerInvulnerabilityDuration;
    }
}

[[nodiscard]] bool ShouldDieByLifetime(const Bullet &b) {
    return b.age >= kPlayerBulletMaxLifetime;
}

void OnSplitterDeath(World &world, std::vector<Enemy>& enemies, const Enemy& splitter) {
    Vector2 direction_to_player = Vector2Normalize(world.player.pos - splitter.pos);

    Vector2 direction_a = {-direction_to_player.y, direction_to_player.x};
    Vector2 direction_b = {direction_to_player.y, -direction_to_player.x};
    const auto& splitter_def = GetEnemyDef(EnemyType::Splitter);
    const float split_offset = splitter_def.radius / 2;
    Vector2 pos_twin_a = Vector2Add(splitter.pos, direction_a * split_offset);
    Vector2 pos_twin_b = Vector2Add(splitter.pos, direction_b * split_offset);
    enemies.emplace_back(pos_twin_a, EnemyType::Twin);
    enemies.emplace_back(pos_twin_b, EnemyType::Twin);
}

void UpdateExp(World &world, const float dt) {
    for (auto &exp: world.exps) {
        if (CheckCollisionCircles(world.player.pos, world.player.magnet_radius, exp.pos, kExpRadius)) {
            exp.pos = Vector2MoveTowards(exp.pos, world.player.pos, kExpSpeed * dt);
        }
        if (CheckCollisionCircleRec(exp.pos, kExpRadius, world.player.rect())) {
            world.current_exp += exp.value;
            exp.dead = true;
        }
    }
}

void UpdateDamagePopups(World &world, const float dt) {
    for (auto &dp: world.damage_popups) {
        dp.pos += dp.vel * dt;
        dp.age += dt;
        if (dp.age >= dp.lifetime) {
            dp.dead = true;
        }
    }
}

void FillLevelUpMenu(World &world) {
    std::array<Upgrade, static_cast<int>(Upgrade::Count)> all{};
    for (int i = 0; i < static_cast<int>(all.size()); ++i) all[i] = static_cast<Upgrade>(i);
    std::shuffle(all.begin(), all.end(), world.rng);
    for (int i = 0; i < 3; ++i) world.level_up_menu.options[i] = all[i];
    world.level_up_menu.selected = 0;
}

void UpdateGameState(World &world) {
    if (world.player.health <= 0.f && world.game_state != GameState::GameOver) {
        world.player.health = 0.f;
        world.game_state = GameState::GameOver;
    }
    else if (TryLevelUp(world)) {
        FillLevelUpMenu(world);
    }
    else {
        world.game_state = GameState::Playing;
    }
}
}

// таймер + новая пуля в ближайшего
void UpdateShooting(World &world, const float dt) {
    Player &p = world.player;
    p.shot_timer += dt;
    if (p.shot_timer < p.fire_interval) return;

    const std::optional<Vector2> target = NearestEnemyPos(world);
    if (!target) {
        p.shot_timer = p.fire_interval;
        return;
    }

    p.shot_timer -= p.fire_interval;
    world.bullets.push_back({p.pos, Vector2Normalize(*target - p.pos) * p.bullet_speed});
}

// полёт, попадания, удаление
void UpdateBullets(World &world, const float dt) {
    std::vector<Enemy> enemies_to_spawn;    //спавнить специальных врагов при смерти других
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
            if (const EnemyDef &e_def = GetEnemyDef(e.type); CheckCollisionCircles(
                b.pos, world.player.bullet_radius, e.pos, e_def.radius)) {
                e.hit_flash_time = kEnemyFlashDuration;
                world.damage_popups.push_back({
                    .pos = e.pos,
                    .vel = Vector2{0.f, -kDamagePopupSpeed},
                    .damage = world.player.bullet_damage,
                    .enemy_type = e.type
                });
                e.health -= world.player.bullet_damage;
                if (e.health <= 0.f) {
                    if (e_def.exp) {
                        world.exps.push_back({e.pos, e_def.exp});
                    }
                    if (e.type == EnemyType::Splitter) {
                        OnSplitterDeath(world, enemies_to_spawn, e);
                    }
                    e.dead = true;
                    b.dead = true;
                    break;
                }
                b.dead = true;
                }
        }
        b.pos += b.vel * dt;
    }
    world.enemies.insert(world.enemies.end(), enemies_to_spawn.begin(), enemies_to_spawn.end());
}

void Update(World &world, const Input& input) {
    world.game_time += kTick;
    UpdateSpawn(world, kTick);
    UpdatePlayer(world.player, input, kTick);
    UpdateEnemies(world, kTick);
    UpdateSeparation(world, kTick);
    UpdateContactDamage(world);
    UpdateShooting(world, kTick);
    UpdateBullets(world, kTick);
    UpdateExp(world, kTick);
    UpdateDamagePopups(world, kTick);

    UpdateGameState(world);

    std::erase_if(world.enemies, [](const Enemy &e) { return e.dead; });
    std::erase_if(world.bullets, [](const Bullet &b) { return b.dead; });
    std::erase_if(world.exps, [](const Exp &e) { return e.dead; });
    std::erase_if(world.damage_popups, [](const DamagePopup &dp) { return dp.dead; });
}

void ApplyUpgrade(Player &p, Upgrade u) {
    // только изменение
    switch (u) {
        case Upgrade::FireInterval: p.fire_interval = std::max(0.1f, p.fire_interval - kUpgradeFireInterval);
            break;
        case Upgrade::MoveSpeed: p.speed += kUpgradeMoveSpeed;
            break;
        case Upgrade::BulletSpeed: p.bullet_speed += kUpgradeBulletSpeed;
            break;
        case Upgrade::Magnet: p.magnet_radius += kUpgradeMagnet;
            break;
        case Upgrade::MaxHealth: p.max_health += kUpgradeMaxHealth;
            p.health += kUpgradeMaxHealth;
            break;
        case Upgrade::RestoreHealth: p.health = std::min(p.health + kUpgradeRestoreHealth, p.max_health);
            break;
        case Upgrade::Count: break;
    }
}

bool UpdateLevelUp(World &world, const Input &input) {
    auto &m = world.level_up_menu;
    auto inp = input.pressed;
    if (inp.card_right) m.selected = (m.selected + 1) % 3;
    if (inp.card_left) m.selected = (m.selected + 3 - 1) % 3;
    const bool mouse_moved = Vector2Length(inp.mouse_delta) > 0.0f;
    bool confirm = inp.card_choose;
    if (inp.picked_card) {
        assert(inp.picked_card.value() < 3);
        m.selected = inp.picked_card.value();
        confirm = true;
    }
    for (int i = 0; i < 3; ++i) {
        if (CheckCollisionPointRec(input.down.mouse_pos, CardRect(i))) {
            if (mouse_moved) m.selected = i;
            if (inp.card_choose_mouse) confirm = true;
        }
    }
    return confirm;
}

[[nodiscard]] bool TryLevelUp(World &world) {
    if (world.current_exp >= world.exp_to_level && world.game_state != GameState::LevelUp) {
        ++world.level;
        const float leftover_exp = world.current_exp - world.exp_to_level;
        world.current_exp = leftover_exp;
        world.exp_to_level *= kExpToLevelMultiplier;
        world.game_state = GameState::LevelUp;
        return true;
    }
    return false;
}

void UpdateByState(World &world, const Input &input) {
    switch (world.game_state) {
        case GameState::Playing:
            Update(world, input);
            break;
        case GameState::LevelUp:
            if (UpdateLevelUp(world, input)) {
                ApplyUpgrade(world.player, world.level_up_menu.options[world.level_up_menu.selected]);
                world.game_state = GameState::Playing;
            }
            break;
        case GameState::GameOver:
            if (input.pressed.restart) {
                world = World{};
            }
            break;
    }
}
