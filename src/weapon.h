#pragma once

#include <raylib.h>
#include <vector>
#include <memory>


constexpr float kBulletMaxLifetime = 10.f;
constexpr float kBulletMinFireInterval = 0.1f;

constexpr float kUpgradeFireInterval = 0.2f;
constexpr float kUpgradeBulletSpeed = 25.f;

struct World;

class Weapon {
public:
    virtual void Update(World &world, const float dt) = 0;
    virtual void Draw(const World &world) const = 0;
    virtual ~Weapon() = default;
};

struct Bullet {
    Vector2 pos;
    Vector2 vel;
    float age = 0.f;
    bool dead = false;
};

class Pistol : public Weapon {
public:
    void Update(World &world, const float dt) override;
    void Draw(const World &world) const override;

    [[nodiscard]] const std::vector<Bullet>& GetBullets() const;

private:
    void UpdateBullets(World &world, const float dt);
    void ApplyDamage(World &world);

    std::vector<Bullet> bullets;

    float damage = 5.f;
    float speed = 300.f;
    float radius = 3.f;
    float fire_interval = 2.f;
    float shot_timer = 0.f;
};

class Aura : public Weapon {
public:
    void Update(World &world, const float dt) override;
    void Draw(const World &world) const override;

private:
    [[nodiscard]] bool TryTriggerAura(const float dt);
    void ApplyDamage(World &world);

    float damage = 2.f;
    float radius = 100.f;
    float fire_interval = 0.7f;
    float shot_timer = 0.f;
};

class Blades : public Weapon {
public:
    void Update(World &world, const float dt) override;
    void Draw(const World &world) const override;

private:
    void UpdateBlades(World &world, const float dt);
    [[nodiscard]] bool TryTriggerBlades(const float dt);
    [[nodiscard]] bool TrySpawnBlades(const float dt);
    void ApplyDamage(World &world);

    std::vector<Vector2> blades_pos;
    float angle = 0.f;
    float damage = 2.f;
    float circle_radius = 130.f;
    float blade_radius = 7.f;
    std::size_t count = 1;
    static constexpr float rotate_speed = 75.f;
    float fire_interval = 0.1f;
    float shot_timer = 0.f;
    float spawn_interval = 2.f;
    float live_interval = 4.f;
    float timer = 0.f;
    bool active = true;
};

[[nodiscard]] std::vector<std::unique_ptr<Weapon>> StartingWeapons();
