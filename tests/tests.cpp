#include <catch2/catch_test_macros.hpp>
#include "world.h"
#include "update.h"

#include <cmath>

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

    CHECK(player.health > health);
    CHECK(player.health == player.max_health);
}

TEST_CASE("ApplyUpgrade: RestoreHealth smaller than missing health -> health grows by restore amount", "[upgrade]") {
    World world;
    Player &player = world.player;
    player.max_health = 100.f;
    const float health = player.max_health - kUpgradeRestoreHealth * 2;
    player.health = health;

    ApplyUpgrade(player, Upgrade::RestoreHealth);

    CHECK(player.health == health * kUpgradeRestoreHealth);
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