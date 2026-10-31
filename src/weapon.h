#pragma once

#include <raylib.h>
#include <vector>
#include <memory>


constexpr float kBulletMaxLifetime = 10.f;
constexpr float kBulletMinFireInterval = 0.1f;

constexpr float kUpgradeFireInterval = 0.2f;
constexpr float kUpgradeBulletSpeed = 25.f;

struct World;

struct PistolLevel {
    float damage = 0.f;
    float speed = 0.f;
    float fire_interval = 0.f;
    std::size_t bullet_count = 0;
};

constexpr PistolLevel kPistolUpgrades[] = {
    { .damage = 5.f,  .speed = 300.f, .fire_interval = 2.f,  .bullet_count = 1 },
    { .damage = 7.f,  .speed = 300.f, .fire_interval = 1.8f, .bullet_count = 1 },
    { .damage = 7.f,  .speed = 300.f, .fire_interval = 1.8f, .bullet_count = 2 },
    { .damage = 10.f, .speed = 500.f, .fire_interval = 1.4f, .bullet_count = 2 },
    { .damage = 10.f, .speed = 500.f, .fire_interval = 1.2f, .bullet_count = 3 },
    { .damage = 15.f, .speed = 500.f, .fire_interval = 1.2f, .bullet_count = 4 },
};

class Cooldown {
public:
    [[nodiscard]] bool Tick(const float interval, const float dt); // Возвращает false если выполняет условие (timer < interval). Если условие не выполняется - интервал списывается, сдача остаётся
    void HoldReady(const float interval);

private:
    float timer_ = 0.f;
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

private:
    void UpdateBullets(World &world, const float dt);
    void ApplyDamage(World &world);

    std::vector<Bullet> bullets_;

    std::size_t level_ = 0;
    float radius_ = 3.f;
    Cooldown cooldown_;
};

class Aura : public Weapon {
public:
    void Update(World &world, const float dt) override;
    void Draw(const World &world) const override;

    static constexpr float kFireInterval = 0.7f;

private:
    void ApplyDamage(World &world);

    float damage_ = 2.f;
    float radius_ = 100.f;
    Cooldown cooldown_;
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

    std::vector<Vector2> blades_pos_;
    float angle_ = 0.f;
    float damage_ = 2.f;
    float blade_radius_ = 7.f;
    std::size_t count_ = 1;
    static constexpr float rotate_speed_ = 75.f;
    Cooldown cooldown_spawn_;
    Cooldown cooldown_hit_;
    bool active_ = false;
};

[[nodiscard]] std::vector<std::unique_ptr<Weapon>> StartingWeapons();
