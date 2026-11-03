#include "weapon.h"

#include "update.h"
#include "world.h"

#include <cmath>
#include <raylib.h>
#include <raymath.h>


namespace {
[[nodiscard]] bool ShouldDieByLifetime(const Bullet &b) {
    return b.age >= Pistol::kMaxRange / b.speed;
}

void MarkBulletByLifetime(Bullet &b) {
    if (ShouldDieByLifetime(b)) {
        b.dead = true;
    }
}

void MoveBullet(Bullet &b, const float dt) {
    b.age += dt;
    b.pos += b.dir * b.speed * dt;
}
}

bool Cooldown::Tick(const float interval, const float dt){
    timer_ += dt;
    if (timer_ < interval) return false;
    timer_ -= interval;
    return true;
}
void Cooldown::HoldReady(const float interval) {
    timer_ = interval;
}

void Cooldown::Reset() {
    timer_ = 0.f;
}

bool Pistol::IsBursting() const {
    return shots_left_ > 0;
}

void Pistol::Fire(World &world) {
    const std::optional<Vector2> target = NearestEnemyPos(world);
    if (!target) {
        fire_cooldown_.HoldReady(kPistolLevels[level_].fire_interval);
        shots_left_ = 0;
        return;
    }
    bullets_.push_back({world.player.pos, Vector2Normalize(*target - world.player.pos), kPistolLevels[level_].speed});
    --shots_left_;
}

void Pistol::UpdateShooting(World &world, const float dt) {
    if (!IsBursting()) {
        if (!fire_cooldown_.Tick(kPistolLevels[level_].fire_interval, dt)) return;
        shots_left_ = kPistolLevels[level_].bullet_count;
        burst_shot_cooldown_.Reset();
        Fire(world);
    }
    else if (burst_shot_cooldown_.Tick(kBurstInterval, dt)) {
        Fire(world);
    }
}

void Pistol::UpdateBullets(const float dt) {
    for (auto &b: bullets_) {
        MoveBullet(b, dt);
        MarkBulletByLifetime(b);
    }
}

void Pistol::ApplyDamage(World &world) {
    for (auto &e: world.enemies) {
        for (auto &b: bullets_) {
            if (e.dead) break;
            if (b.dead) continue;
            if (CheckCollisionCircles(b.pos, radius_, e.pos, GetEnemyDef(e.type).radius)) {
                DamageEnemy(world, e, kPistolLevels[level_].damage);
                b.dead = true;
            }
        }
    }
}

[[nodiscard]] const std::vector<Bullet>& Pistol::GetBullets() const {
    return bullets_;
}

void Pistol::Update(World &world, const float dt) {
    UpdateBullets(dt);
    UpdateShooting(world, dt);
    ApplyDamage(world);
    std::erase_if(bullets_, [](const Bullet &b) { return b.dead; });
}

void Pistol::LevelUp() {
    if (std::size(kPistolLevels) - 1 == level_) return;
    ++level_;
}

WeaponId Pistol::GetId() const {
    return WeaponId::Pistol;
}

void Aura::ApplyDamage(World &world) {
    for (auto &e: world.enemies) {
        if (CheckCollisionCircles(world.player.pos, kAuraLevels[level_].radius, e.pos, GetEnemyDef(e.type).radius)) {
            DamageEnemy(world, e, kAuraLevels[level_].damage);
        }
    }
}

void Aura::Update(World &world, const float dt) {
    if (!cooldown_.Tick(kFireInterval, dt)) return;
    ApplyDamage(world);
}

void Aura::LevelUp() {
    if (std::size(kAuraLevels) - 1 == level_) return;
    ++level_;
}

WeaponId Aura::GetId() const {
    return WeaponId::Aura;
}

void Blades::UpdateBlades(World &world, const float dt) {
    angle_ += rotate_speed_ * dt;
    float step = 360.f / static_cast<float>(kBladesLevels[level_].count);
    for (std::size_t i = 0; i < kBladesLevels[level_].count; ++i) {
        float angle_rad = (angle_ + step * i) * DEG2RAD;
        float cos_rad = std::cos(angle_rad);
        float sin_rad = std::sin(angle_rad);
        Vector2 point = { world.player.pos.x + cos_rad*kCircleRadius, world.player.pos.y - sin_rad*kCircleRadius };
        blades_pos_.push_back(point);
    }
}

[[nodiscard]] bool Blades::TrySpawnBlades(const float dt) {
    const float interval = active_ ? kLiveInterval : kSpawnInterval;
    if (cooldown_spawn_.Tick(interval, dt)) {
        active_ = !active_;
    }
    return active_;
}

void Blades::ApplyDamage(World &world) {
    for (auto &e: world.enemies) {
        for (auto &pos: blades_pos_) {
            if (CheckCollisionCircles(pos, blade_radius_, e.pos, GetEnemyDef(e.type).radius)) {
                DamageEnemy(world, e, kBladesLevels[level_].damage);
            }
        }
    }
}

void Blades::Update(World &world, const float dt) {
    blades_pos_.clear();
    if (!TrySpawnBlades(dt)) return;
    UpdateBlades(world, dt);
    if (!cooldown_hit_.Tick(kFireInterval, dt)) return;
    ApplyDamage(world);
}

void Blades::LevelUp() {
    if (std::size(kBladesLevels) - 1 == level_) return;
    ++level_;
}

WeaponId Blades::GetId() const {
    return WeaponId::Blades;
}

bool Blades::IsActive() const {
    return active_;
}

void Pistol::Draw(const World &) const {
    for (const auto &b: bullets_) {
        DrawCircleV(b.pos, radius_, YELLOW);
    }
}

void Aura::Draw(const World &world) const {
    DrawCircleLinesV(world.player.pos, kAuraLevels[level_].radius, GOLD);
}

void Blades::Draw(const World &) const {
    for (const auto& pos : blades_pos_) {
        DrawCircleV(pos, blade_radius_, BLUE);
    }
}

[[nodiscard]] std::vector<std::unique_ptr<Weapon>> StartingWeapons() {
    std::vector<std::unique_ptr<Weapon>> weapons;
    weapons.push_back(std::make_unique<Pistol>());
    weapons.push_back(std::make_unique<Aura>());
    weapons.push_back(std::make_unique<Blades>());
    return weapons;
}
