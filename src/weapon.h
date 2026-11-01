#pragma once

#include <raylib.h>

#include <vector>
#include <memory>


enum class WeaponId { Pistol, Aura, Blades };

struct World;

struct PistolLevel {
    float damage = 0.f;
    float speed = 0.f;
    float fire_interval = 0.f;
    std::size_t bullet_count = 0;
};

struct AuraLevel {
    float damage = 0.f;
    float radius = 0.f;
};

struct BladesLevel {
    float damage = 0.f;
    std::size_t count = 0;
};

class Cooldown {
public:
    // Возвращает false если выполняет условие (timer < interval). Если условие не выполняется - интервал списывается, сдача остаётся
    [[nodiscard]] bool Tick(const float interval, const float dt);
    void HoldReady(const float interval);

private:
    float timer_ = 0.f;
};

class Weapon {
public:
    virtual void Update(World &world, const float dt) = 0;
    virtual void Draw(const World &world) const = 0;
    virtual void LevelUp() = 0;
    virtual WeaponId GetId() const = 0;

    virtual ~Weapon() = default;

    static constexpr float kBulletMinFireInterval = 0.1f;
};

struct Bullet {
    Vector2 pos;
    Vector2 dir;
    float speed = 0.f;
    float age = 0.f;
    bool dead = false;
};

class Pistol : public Weapon {
public:
    void Update(World &world, const float dt) override;
    void Draw(const World &world) const override;
    void LevelUp() override;
    [[nodiscard]] WeaponId GetId() const override;
    [[nodiscard]] const std::vector<Bullet>& GetBullets() const;

    static constexpr PistolLevel kPistolLevels[] = {
        { .damage = 5.f,  .speed = 300.f, .fire_interval = 2.f,  .bullet_count = 1 },
        { .damage = 7.f,  .speed = 300.f, .fire_interval = 1.8f, .bullet_count = 1 },
        { .damage = 7.f,  .speed = 300.f, .fire_interval = 1.8f, .bullet_count = 2 },
        { .damage = 10.f, .speed = 500.f, .fire_interval = 1.4f, .bullet_count = 2 },
        { .damage = 10.f, .speed = 500.f, .fire_interval = 1.2f, .bullet_count = 3 },
        { .damage = 15.f, .speed = 500.f, .fire_interval = 1.2f, .bullet_count = 4 }
    };

    static constexpr float max_range_ = 800.f;
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
    void LevelUp() override;
    [[nodiscard]] WeaponId GetId() const override;

    static constexpr float kFireInterval = 0.7f;
    static constexpr AuraLevel kAuraLevels[] = {
        { .damage = 2.f,  .radius = 100.f, },
        { .damage = 3.f,  .radius = 100.f, },
        { .damage = 5.f,  .radius = 100.f, },
        { .damage = 5.f,  .radius = 125.f, },
        { .damage = 6.f,  .radius = 150.f, },
        { .damage = 10.f, .radius = 150.f, }
    };

private:
    void ApplyDamage(World &world);

    std::size_t level_ = 0;
    Cooldown cooldown_;
};

class Blades : public Weapon {
public:
    void Update(World &world, const float dt) override;
    void Draw(const World &world) const override;
    void LevelUp() override;
    [[nodiscard]] WeaponId GetId() const override;
    [[nodiscard]] bool IsActive() const;

    static constexpr float kFireInterval = 0.1f;
    static constexpr float kSpawnInterval = 2.f;
    static constexpr float kLiveInterval = 4.f;
    static constexpr float kCircleRadius = 130.f;
    static constexpr BladesLevel kBladesLevels[] = {
        { .damage = 2.f,  .count = 1, },
        { .damage = 3.f,  .count = 2, },
        { .damage = 5.f,  .count = 3, },
        { .damage = 5.f,  .count = 4, },
        { .damage = 6.f,  .count = 5, },
        { .damage = 10.f, .count = 6, }
    };

private:
    void UpdateBlades(World &world, const float dt);
    [[nodiscard]] bool TrySpawnBlades(const float dt);
    void ApplyDamage(World &world);

    std::vector<Vector2> blades_pos_;

    float angle_ = 0.f;
    float blade_radius_ = 7.f;
    static constexpr float rotate_speed_ = 75.f;
    std::size_t level_ = 0;
    Cooldown cooldown_spawn_;
    Cooldown cooldown_hit_;
    bool active_ = false;
};

[[nodiscard]] std::vector<std::unique_ptr<Weapon>> StartingWeapons();
