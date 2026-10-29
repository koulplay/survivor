#include "weapon.h"
#include "update.h"
#include "world.h"

#include <raylib.h>
#include <raymath.h>
#include <cmath>

namespace {
[[nodiscard]] bool ShouldDieByLifetime(const Bullet &b) {
    return b.age >= kBulletMaxLifetime;
}

void DeleteBulletByLifetime(Bullet &b) {
    if (ShouldDieByLifetime(b)) {
        b.dead = true;
    }
}

void MoveBullet(Bullet &b, const float dt) {
    b.age += dt;
    b.pos += b.vel * dt;
}
}


void Pistol::UpdateBullets(World &world, const float dt) {
    for (auto &b: bullets) {
        DeleteBulletByLifetime(b);
        MoveBullet(b, dt);
    }
    shot_timer += dt;
    float interval = std::max(kBulletMinFireInterval, kBulletFireInterval - world.player.bullet_fire_interval_upgrade);
    if (shot_timer < interval) return;

    const std::optional<Vector2> target = NearestEnemyPos(world);
    if (!target) {
        shot_timer = interval;
        return;
    }
    shot_timer -= interval;
    bullets.push_back({world.player.pos, Vector2Normalize(*target - world.player.pos) * (speed + world.player.bullet_speed_upgrade)});
}

void Pistol::ApplyDamage(World &world) {
    for (auto &e: world.enemies) {
        for (auto &b: bullets) {
            if (e.dead) break;
            if (b.dead) continue;
            if (CheckCollisionCircles(b.pos, radius, e.pos, GetEnemyDef(e.type).radius)) {
                DamageEnemy(world, e, damage);
                b.dead = true;
            }
        }
    }
}

[[nodiscard]] const std::vector<Bullet>& Pistol::GetBullets() const {
    return bullets;
}

void Pistol::Update(World &world, const float dt) {
    UpdateBullets(world, dt);
    ApplyDamage(world);
    std::erase_if(bullets, [](const Bullet &b) { return b.dead; });
}

[[nodiscard]] bool Aura::TryTriggerAura(const float dt) {
    shot_timer += dt;
    if (shot_timer < kAuraFireInterval) return false;
    shot_timer -= kAuraFireInterval;
    return true;
}

void Aura::ApplyDamage(World &world) {
    for (auto &e: world.enemies) {
        if (CheckCollisionCircles(world.player.pos, radius, e.pos, GetEnemyDef(e.type).radius)) {
            DamageEnemy(world, e, damage);
        }
    }
}

void Aura::Update(World &world, const float dt) {
    if (!TryTriggerAura(dt)) return;
    ApplyDamage(world);
}

void Blades::UpdateBlades(World &world, const float dt) {
    angle += rotate_speed * dt;
    float step = 360.f / static_cast<float>(count);
    for (std::size_t i = 0; i < count; ++i) {
        float angle_rad = (angle + step * i) * DEG2RAD;
        float cos_rad = std::cos(angle_rad);
        float sin_rad = std::sin(angle_rad);
        Vector2 point = { world.player.pos.x + cos_rad*circle_radius, world.player.pos.y - sin_rad*circle_radius };
        blades_pos.push_back(point);
    }
}

[[nodiscard]] bool Blades::TrySpawnBlades(const float dt) {
    timer += dt;
    const float interval = active ? live_interval : spawn_interval;
    if (timer >= interval) {
        timer -= interval;
        active = !active;
    }
    return active;
}

[[nodiscard]] bool Blades::TryTriggerBlades(const float dt) {
    shot_timer += dt;
    if (shot_timer < kBladesFireInterval) return false;
    shot_timer -= kBladesFireInterval;
    return true;
}

void Blades::ApplyDamage(World &world) {
    for (auto &e: world.enemies) {
        for (auto &pos: blades_pos) {
            if (CheckCollisionCircles(pos, blade_radius, e.pos, GetEnemyDef(e.type).radius)) {
                DamageEnemy(world, e, damage);
            }
        }
    }
}

void Blades::Update(World &world, const float dt) {
    blades_pos.clear();
    if (!TrySpawnBlades(dt)) return;
    UpdateBlades(world, dt);
    if (!TryTriggerBlades(dt)) return;
    ApplyDamage(world);
}


void Pistol::Draw(const World &) const {
    for (const auto &b: bullets) {
        DrawCircleV(b.pos, radius, YELLOW);
    }
}

void Aura::Draw(const World &world) const {
    DrawCircleLinesV(world.player.pos, radius, GOLD);
}

void Blades::Draw(const World &) const {
    for (const auto& pos : blades_pos) {
        DrawCircleV(pos, blade_radius, BLUE);
    }
}

[[nodiscard]] std::vector<std::unique_ptr<Weapon>> StartingWeapons() {
    std::vector<std::unique_ptr<Weapon>> weapons;
    weapons.push_back(std::make_unique<Pistol>());
    weapons.push_back(std::make_unique<Aura>());
    weapons.push_back(std::make_unique<Blades>());
    return weapons;
}
