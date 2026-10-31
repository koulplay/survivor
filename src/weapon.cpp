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

bool Cooldown::Tick(const float interval, const float dt){
    timer += dt;
    if (timer < interval) return false;
    timer -= interval;
    return true;
}
void Cooldown::HoldReady(const float interval) {
    timer = interval;
}

void Pistol::UpdateBullets(World &world, const float dt) {
    for (auto &b: bullets) {
        DeleteBulletByLifetime(b);
        MoveBullet(b, dt);
    }
    const float interval = std::max(kBulletMinFireInterval, kFireInterval - world.player.bullet_fire_interval_upgrade);
    if (!cooldown.Tick(interval, dt)) return;

    const std::optional<Vector2> target = NearestEnemyPos(world);
    if (!target) {
        cooldown.HoldReady(interval);
        return;
    }
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

void Aura::ApplyDamage(World &world) {
    for (auto &e: world.enemies) {
        if (CheckCollisionCircles(world.player.pos, radius, e.pos, GetEnemyDef(e.type).radius)) {
            DamageEnemy(world, e, damage);
        }
    }
}

void Aura::Update(World &world, const float dt) {
    if (!cooldown.Tick(kFireInterval, dt)) return;
    ApplyDamage(world);
}

void Blades::UpdateBlades(World &world, const float dt) {
    angle += rotate_speed * dt;
    float step = 360.f / static_cast<float>(count);
    for (std::size_t i = 0; i < count; ++i) {
        float angle_rad = (angle + step * i) * DEG2RAD;
        float cos_rad = std::cos(angle_rad);
        float sin_rad = std::sin(angle_rad);
        Vector2 point = { world.player.pos.x + cos_rad*kCircleRadius, world.player.pos.y - sin_rad*kCircleRadius };
        blades_pos.push_back(point);
    }
}

[[nodiscard]] bool Blades::TrySpawnBlades(const float dt) {
    const float interval = active ? kLiveInterval : kSpawnInterval;
    if (cooldown_spawn.Tick(interval, dt)) {
        active = !active;
    }
    return active;
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
    if (!cooldown_hit.Tick(kFireInterval, dt)) return;
    ApplyDamage(world);
}

bool Blades::IsActive() const {
    return active;
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
