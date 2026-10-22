#include <catch2/catch_test_macros.hpp>
#include "world.h"
#include "update.h"

#include <cmath>
#include <bit>

struct Fnv1a {
    std::uint64_t value = 14695981039346656037ull; // offset basis (RFC 9923)

    void Byte(std::uint8_t b) {
        value ^= b;
        value *= 1099511628211ull; // FNV prime; беззнаковое переполнение — по модулю 2^64
    }

    void U32(std::uint32_t v) {
        // младший байт первым — на любом процессоре одинаково
        for (int i = 0; i < 4; ++i) Byte(static_cast<std::uint8_t>(v >> (8 * i)));
    }

    void U64(std::uint64_t v) {
        for (int i = 0; i < 8; ++i) Byte(static_cast<std::uint8_t>(v >> (8 * i)));
    }

    void F32(float f) { U32(std::bit_cast<std::uint32_t>(f)); }
    void F64(double f) { U64(std::bit_cast<std::uint64_t>(f)); }
    void Bool(bool b) { Byte(b ? 1 : 0); }
};

void AddPlayer(Fnv1a &hash, const Player &player) {
    hash.F32(player.pos.x);
    hash.F32(player.pos.y);
    hash.F32(player.size);
    hash.F32(player.speed);
    hash.F32(player.fire_interval);
    hash.F32(player.bullet_radius);
    hash.F32(player.bullet_speed);
    hash.F32(player.bullet_damage);
    hash.F32(player.health);
    hash.F32(player.max_health);
    hash.F32(player.shot_timer);
    hash.F32(player.magnet_radius);
    hash.F32(player.invulnerability_timer);
}

void AddEnemies(Fnv1a &hash, const std::vector<Enemy> &enemies) {
    hash.U64(enemies.size());
    for (const Enemy &e: enemies) {
        hash.F32(e.pos.x);
        hash.F32(e.pos.y);
        hash.U32(static_cast<uint32_t>(e.type));
        hash.F32(e.health);
    }
}

void AddBullets(Fnv1a &hash, const std::vector<Bullet> &bullets) {
    hash.U64(bullets.size());
    for (const Bullet &b: bullets) {
        hash.F32(b.pos.x);
        hash.F32(b.pos.y);
        hash.F32(b.vel.x);
        hash.F32(b.vel.y);
        hash.F32(b.age);
    }
}

void AddExp(Fnv1a &hash, const std::vector<Exp> &exps) {
    hash.U64(exps.size());
    for (const Exp &e: exps) {
        hash.F32(e.pos.x);
        hash.F32(e.pos.y);
        hash.F32(e.value);
    }
}

void AddLevelUpMenu(Fnv1a &hash, const LevelUpMenu &level_up_menu) {
    for (const Upgrade &u: level_up_menu.options) {
        hash.U32(static_cast<uint32_t>(u));
    }
    hash.U32(level_up_menu.selected);
}

void AddGameToHash(Fnv1a &hash, const World &world) {
    AddPlayer(hash, world.player);
    AddEnemies(hash, world.enemies);
    AddBullets(hash, world.bullets);
    AddExp(hash, world.exps);
    AddLevelUpMenu(hash, world.level_up_menu);
    hash.U32(static_cast<uint32_t>(world.game_state));
    hash.F32(world.current_exp);
    hash.F32(world.exp_to_level);
    hash.F32(world.enemy_spawn_timer);
    hash.F64(world.game_time);
}

void Scenario(const World &world, Input &input, const size_t tick) {
    input = Input();

    if (tick > 0) input.down.player_move_down = true;
    if (tick > 0) input.down.player_move_left = true;
    if (world.game_state == GameState::LevelUp) input.pressed.card_choose = true;
    if (world.game_state == GameState::GameOver) input.pressed.restart = true;
}

TEST_CASE("UpdateByState: two world created with seed 42 with same input scenario for 3600 ticks -> the hashes match every 60 ticks", "[hash]") {
    World w1(42);
    World w2(42);
    Input input1;
    Input input2;
    constexpr size_t max_ticks = 3600;

    for (size_t tick = 0; tick < max_ticks; ++tick) {
        Scenario(w1, input1, tick);
        Scenario(w2, input2, tick);
        UpdateByState(w1, input1);
        UpdateByState(w2, input2);
        if (w1.game_state == GameState::GameOver && input1.pressed.restart) w1 = World(43);
        if (w2.game_state == GameState::GameOver && input2.pressed.restart) w2 = World(43);

        if (tick % 60 == 0) {
            Fnv1a hash1;
            Fnv1a hash2;
            AddGameToHash(hash1, w1);
            AddGameToHash(hash2, w2);

            CAPTURE(tick);
            CHECK(hash1.value == hash2.value);
            CHECK(w1.rng == w2.rng);
        }
    }
    CHECK(w1.game_state == GameState::Playing);
}


TEST_CASE("TryLevelUp: exp exactly at threshold -> level rises", "[level]") {
    World world;
    world.exp_to_level = 1.f;
    world.current_exp = 1.f;

    REQUIRE(TryLevelUp(world));
    CHECK(world.level == 1);
    CHECK(world.current_exp == 0.f);
    CHECK(world.exp_to_level == 1.f * kExpToLevelMultiplier);
    CHECK(world.game_state == GameState::LevelUp);
}

TEST_CASE("TryLevelUp: exp above threshold -> level rises and leftover carries over", "[level]") {
    World world;
    world.exp_to_level = 1.f;
    world.current_exp = 1.5f;

    REQUIRE(TryLevelUp(world));
    CHECK(world.level == 1);
    CHECK(world.current_exp == 0.5f);
    CHECK(world.exp_to_level == 1.f * kExpToLevelMultiplier);
    CHECK(world.game_state == GameState::LevelUp);
}

TEST_CASE("TryLevelUp: exp below threshold -> nothing changes", "[level]") {
    World world;
    world.exp_to_level = 1.f;
    world.current_exp = 0.5f;

    REQUIRE_FALSE(TryLevelUp(world));
    CHECK(world.level == 0);
    CHECK(world.current_exp == 0.5f);
    CHECK(world.exp_to_level == 1.f);
    CHECK(world.game_state == GameState::Playing);
}

TEST_CASE("UpdateShooting: timer past interval with enemy -> shoots and keeps leftover", "[shooting]") {
    World world;
    Player &player = world.player;
    player.fire_interval = 2.f;
    player.shot_timer = 0.f;
    world.enemies.emplace_back(world.player.pos, EnemyType::Light);

    UpdateShooting(world, 3.f);

    CHECK(world.bullets.size() == 1);
    CHECK(player.shot_timer == 1.f);
}

TEST_CASE("UpdateShooting: timer exactly at interval -> shoots and timer is zero", "[shooting]") {
    World world;
    Player &player = world.player;
    player.fire_interval = 2.f;
    player.shot_timer = 0.f;
    world.enemies.emplace_back(world.player.pos, EnemyType::Light);

    UpdateShooting(world, 2.f);

    CHECK(world.bullets.size() == 1);
    CHECK(player.shot_timer == 0.f);
}

TEST_CASE("UpdateShooting: timer below interval -> no shot and timer keeps counting", "[shooting]") {
    World world;
    Player &player = world.player;
    player.fire_interval = 2.f;
    player.shot_timer = 0.f;
    world.enemies.emplace_back(world.player.pos, EnemyType::Light);

    UpdateShooting(world, 1.f);

    CHECK(world.bullets.size() == 0);
    CHECK(player.shot_timer == 1.f);
}

TEST_CASE("UpdateShooting: timer ready without enemies -> no shot and timer held at interval", "[shooting]") {
    World world;
    Player &player = world.player;
    player.fire_interval = 2.f;
    player.shot_timer = 0.f;

    UpdateShooting(world, 3.f);

    CHECK(world.bullets.size() == 0);
    CHECK(player.shot_timer == player.fire_interval);
}

TEST_CASE("ApplyUpgrade: RestoreHealth larger than missing health -> health capped at max", "[upgrade]") {
    World world;
    Player &player = world.player;
    player.max_health = 100.f;
    const float health = player.max_health - kUpgradeRestoreHealth / 2;
    player.health = health;

    ApplyUpgrade(player, Upgrade::RestoreHealth);

    CHECK(player.health == player.max_health);
}

TEST_CASE("ApplyUpgrade: RestoreHealth smaller than missing health -> health grows by restore amount", "[upgrade]") {
    World world;
    Player &player = world.player;
    player.max_health = 100.f;
    const float health = player.max_health - kUpgradeRestoreHealth * 2;
    player.health = health;

    ApplyUpgrade(player, Upgrade::RestoreHealth);

    CHECK(player.health == health + kUpgradeRestoreHealth);
}

TEST_CASE("UpdateBullets: hit drops enemy health to exactly 0 -> enemy dies", "[enemy]") {
    World world;
    Enemy &enemy = world.enemies.emplace_back(world.player.pos, EnemyType::Light);
    world.bullets.emplace_back(world.player.pos, Vector2{});
    world.player.bullet_damage = 1.f;
    enemy.health = 1.f;

    UpdateBullets(world, 2.f);

    CHECK(enemy.dead);
}

TEST_CASE("UpdateBullets: hit drops enemy health below 0 -> enemy dies", "[enemy]") {
    World world;
    Enemy &enemy = world.enemies.emplace_back(world.player.pos, EnemyType::Light);
    world.bullets.emplace_back(world.player.pos, Vector2{});
    world.player.bullet_damage = 10.f;
    enemy.health = 1.f;

    UpdateBullets(world, 2.f);

    CHECK(enemy.dead);
}
