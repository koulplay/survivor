#pragma once

#include <raylib.h>
#include <vector>
#include <memory>


constexpr float kBulletMaxLifetime = 10.f;
constexpr float kBulletMinFireInterval = 0.1f;

constexpr float kUpgradeFireInterval = 0.2f;
constexpr float kUpgradeBulletSpeed = 25.f;

struct World;

class Cooldown {
public:
    [[nodiscard]] bool Tick(const float interval, const float dt); // Возвращает false если выполняет условие (timer <= interval). Если условие не выполняется - интервал списывается, сдача остаётся
    void HoldReady(const float interval);

private:
    float timer = 0.f;
};

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

    static constexpr float kFireInterval = 2.f;

private:
    void UpdateBullets(World &world, const float dt);
    void ApplyDamage(World &world);

    std::vector<Bullet> bullets;

    float damage = 5.f;
    float speed = 300.f;
    float radius = 3.f;
    Cooldown cooldown;
};

class Aura : public Weapon {
public:
    void Update(World &world, const float dt) override;
    void Draw(const World &world) const override;

    static constexpr float kFireInterval = 0.7f;

private:
    void ApplyDamage(World &world);

    float damage = 2.f;
    float radius = 100.f;
    Cooldown cooldown;
};

class Blades : public Weapon {
public:
    void Update(World &world, const float dt) override;
    void Draw(const World &world) const override;
    [[nodiscard]] bool IsActive() const;

    static constexpr float kFireInterval = 0.1f;
    static constexpr float kSpawnInterval = 2.f;
    static constexpr float kLiveInterval = 4.f;
    static constexpr float kCircleRadius = 130.f;

private:
    void UpdateBlades(World &world, const float dt);
    [[nodiscard]] bool TrySpawnBlades(const float dt);
    void ApplyDamage(World &world);

    std::vector<Vector2> blades_pos;
    float angle = 0.f;
    float damage = 2.f;
    float blade_radius = 7.f;
    std::size_t count = 1;
    static constexpr float rotate_speed = 75.f;
    Cooldown cooldown_spawn;
    Cooldown cooldown_hit;
    bool active = false;
};

[[nodiscard]] std::vector<std::unique_ptr<Weapon>> StartingWeapons();
