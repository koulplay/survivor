#include <catch2/catch_test_macros.hpp>
#include "world.h"
#include "update.h"

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