#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include "world.h"
#include "update.h"

#include <cmath>
#include <bit>
#include <raymath.h>

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
    hash.F32(player.health);
    hash.F32(player.max_health);
    hash.F32(player.size);
    hash.F32(player.speed);
    hash.F32(player.magnet_radius);
    hash.F32(player.bullet_speed_upgrade);
    hash.F32(player.bullet_fire_interval_upgrade);
    hash.F32(player.invulnerability_timer);
    hash.U64(player.weapons.size());
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

TEST_CASE("Pistol::Update: timer past interval with enemy -> shoots and keeps leftover", "[weapon]") {
    World world;
    world.player.pos = {0.f, 0.f};
    world.enemies.emplace_back(Vector2{0.f, 10'000.f}, EnemyType::Light);
    Pistol pistol;
    constexpr float leftover = Pistol::kFireInterval / 2.f;

    pistol.Update(world, Pistol::kFireInterval + leftover);
    REQUIRE(pistol.GetBullets().size() == 1);

    pistol.Update(world, Pistol::kFireInterval - leftover);

    CHECK(pistol.GetBullets().size() == 2);
}

TEST_CASE("Pistol::Update: timer exactly at interval -> shoots and timer is zero", "[weapon]") {
    World world;
    world.player.pos = {0.f, 0.f};
    world.enemies.emplace_back(Vector2{0.f, 10'000.f}, EnemyType::Light);
    Pistol pistol;
    constexpr float half_interval = Pistol::kFireInterval / 2.f;

    pistol.Update(world, Pistol::kFireInterval);
    REQUIRE(pistol.GetBullets().size() == 1);

    pistol.Update(world, half_interval);

    CHECK(pistol.GetBullets().size() == 1);
}

TEST_CASE("Pistol::Update: timer below interval -> no shot", "[weapon]") {
    World world;
    world.player.pos = {0.f, 0.f};
    world.enemies.emplace_back(Vector2{0.f, 10'000.f}, EnemyType::Light);
    Pistol pistol;
    constexpr float half_interval = Pistol::kFireInterval / 2.f;

    pistol.Update(world, half_interval);

    CHECK(pistol.GetBullets().size() == 0);
}

TEST_CASE("Pistol::Update: timer ready without enemies -> no shot and timer held at interval", "[weapon]") {
    World world;
    world.player.pos = {0.f, 0.f};
    Pistol pistol;

    pistol.Update(world, Pistol::kFireInterval);
    REQUIRE(pistol.GetBullets().size() == 0);

    world.enemies.emplace_back(Vector2{0.f, 10'000.f}, EnemyType::Light);
    pistol.Update(world, 0.f);

    CHECK(pistol.GetBullets().size() == 1);
}

TEST_CASE("Aura::Update: timer past interval -> aura triggers and keeps leftover", "[weapon]") {
    World world;
    world.player.pos = {0.f, 0.f};
    Enemy &enemy = world.enemies.emplace_back(Vector2{0.f, 0.f}, EnemyType::Light);
    constexpr float base_enemy_health = 100.f;
    enemy.health = base_enemy_health;
    Aura aura;
    constexpr float leftover = Aura::kFireInterval / 2.f;

    aura.Update(world, Aura::kFireInterval + leftover);
    const float damage = base_enemy_health - enemy.health;

    REQUIRE(enemy.health < base_enemy_health);

    aura.Update(world, Aura::kFireInterval - leftover / 2);

    CHECK(enemy.health == base_enemy_health - damage * 2); // Проверка на корректное срабатывание нанесения урона за два удара
}

TEST_CASE("Aura::Update: timer exactly at interval -> aura triggers and timer is zero", "[weapon]") {
    World world;
    world.player.pos = {0.f, 0.f};
    Enemy &enemy = world.enemies.emplace_back(Vector2{0.f, 0.f}, EnemyType::Light);
    constexpr float base_enemy_health = 100.f;
    enemy.health = base_enemy_health;
    Aura aura;

    aura.Update(world, Aura::kFireInterval);
    REQUIRE(enemy.health < base_enemy_health);
    const float damage = base_enemy_health - enemy.health;

    aura.Update(world, 0.f);

    CHECK(enemy.health == base_enemy_health - damage);
}

TEST_CASE("Aura::Update: timer below interval -> does nothing", "[weapon]") {
    World world;
    world.player.pos = {0.f, 0.f};
    Enemy &enemy = world.enemies.emplace_back(Vector2{0.f, 0.f}, EnemyType::Light);
    constexpr float base_enemy_health = 100.f;
    enemy.health = base_enemy_health;
    Aura aura;

    aura.Update(world, Aura::kFireInterval / 2.f);

    CHECK(enemy.health == base_enemy_health);
}

TEST_CASE("Aura::Update: enemy out of aura range -> enemy survives", "[weapon]") {
    World world;
    world.player.pos = {0.f, 0.f};
    Enemy &enemy = world.enemies.emplace_back(Vector2{0.f, 10'000.f}, EnemyType::Light);
    constexpr float base_enemy_health = 1.f;
    enemy.health = base_enemy_health;
    Aura aura;

    aura.Update(world, Aura::kFireInterval);

    CHECK(enemy.health == base_enemy_health);
}

void SpawnEnemiesInBladesRadius(World &world) {
    for (std::size_t i = 0; i < 360; ++i) {
        const float angle_rad = i * DEG2RAD;
        const float cos_rad = std::cos(angle_rad);
        const float sin_rad = std::sin(angle_rad);
        Vector2 point = { world.player.pos.x + cos_rad * Blades::kCircleRadius, world.player.pos.y - sin_rad * Blades::kCircleRadius };
        world.enemies.emplace_back(point, EnemyType::Light);
    }
}

void MakeBladesActive(World &world, Blades &blades) {
    const auto max_ticks = static_cast<std::size_t>(std::ceil(Blades::kSpawnInterval / kTick)) + 1; //Добавляем кадр для активации клинков
    for (std::size_t tick = 0; tick < max_ticks && !blades.IsActive(); ++tick) {
        blades.Update(world, kTick);
    }
    REQUIRE(blades.IsActive());
}

TEST_CASE("Blades::Update: blades active -> hit enemy", "[weapon]") {
    World world;
    world.player.pos = {0.f, 0.f};
    SpawnEnemiesInBladesRadius(world);
    Blades blades;
    MakeBladesActive(world, blades);

    blades.Update(world, Blades::kFireInterval);

    CHECK(std::ranges::any_of(world.enemies, [](const Enemy& e) { return e.health < GetEnemyDef(e.type).max_health; }));
}

TEST_CASE("Blades::Update: blades not active -> does nothing", "[weapon]") {
    World world;
    world.player.pos = {0.f, 0.f};
    SpawnEnemiesInBladesRadius(world);
    Blades blades;

    blades.Update(world, Blades::kSpawnInterval / 2.f);

    CHECK(std::ranges::none_of(world.enemies, [](const Enemy& e) { return e.health < GetEnemyDef(e.type).max_health; }));
}

TEST_CASE("Blades::Update: timer below interval -> does nothing", "[weapon]") {
    World world;
    world.player.pos = {0.f, 0.f};
    SpawnEnemiesInBladesRadius(world);
    Blades blades;
    constexpr float half_interval = Blades::kFireInterval / 2.f;
    MakeBladesActive(world, blades);

    blades.Update(world, half_interval);

    CHECK_FALSE(std::ranges::any_of(world.enemies, [](const Enemy& e) { return e.health < GetEnemyDef(e.type).max_health; }));
}

TEST_CASE("DamageEnemy: hit drops enemy health to exactly 0 -> enemy dies", "[enemy]") {
    World world;
    Enemy &enemy = world.enemies.emplace_back(Vector2{0.f, 0.f}, EnemyType::Light);
    enemy.health = 5.f;

    DamageEnemy(world, enemy, 5.f);

    CHECK(enemy.dead);
}

TEST_CASE("DamageEnemy: hit drops enemy health below 0 -> enemy dies", "[enemy]") {
    World world;
    Enemy &enemy = world.enemies.emplace_back(Vector2{0.f, 0.f}, EnemyType::Light);
    enemy.health = 1.f;

    DamageEnemy(world, enemy, 5.f);

    CHECK(enemy.dead);
}

TEST_CASE("DamageEnemy: hit drops enemy health above 0 -> enemy survives", "[enemy]") {
    World world;
    Enemy &enemy = world.enemies.emplace_back(Vector2{0.f, 0.f}, EnemyType::Light);
    enemy.health = 100.f;

    DamageEnemy(world, enemy, 5.f);

    CHECK_FALSE(enemy.dead);
}

TEST_CASE("DamageEnemy: hit drops splitter hp above 0 -> splitter alive and no twins spawn", "[enemy]") {
    World world;
    Enemy &enemy = world.enemies.emplace_back(world.player.pos, EnemyType::Splitter);
    enemy.health = 100.f;

    DamageEnemy(world, enemy, 5.f);
    AddNewEnemies(world);

    CHECK_FALSE(world.enemies[0].dead);
    CHECK(world.enemies.size() == 1);
}

TEST_CASE("DamageEnemy: hit kills splitter -> splitter dead and twins spawn", "[enemy]") {
    World world;
    Enemy &enemy = world.enemies.emplace_back(world.player.pos, EnemyType::Splitter);
    enemy.health = 1.f;

    DamageEnemy(world, enemy, 1.f);
    AddNewEnemies(world);

    CHECK(world.enemies[0].dead);
    CHECK(world.enemies.size() == 3);
}

TEST_CASE("DamageEnemy: splitter dead -> no exp spawn", "[enemy]") {
    World world;
    Enemy &enemy = world.enemies.emplace_back(world.player.pos, EnemyType::Splitter);
    enemy.health = 1.f;

    DamageEnemy(world, enemy, 1.f);

    CHECK(world.exps.size() == 0);
}

TEST_CASE("DamageEnemy: hit kills splitter -> twins spawn perpendicular to player", "[enemy]") {
    World world;
    world.player.pos = {0.f, 0.f};
    Enemy &enemy = world.enemies.emplace_back(Vector2{300.f, 400.f}, EnemyType::Splitter);
    enemy.health = 1.f;

    DamageEnemy(world, enemy, 1.f);
    AddNewEnemies(world);

    std::vector<Vector2> twins;
    for (const Enemy &e : world.enemies) {
        if (e.type == EnemyType::Twin) twins.push_back(e.pos);
    }
    REQUIRE(twins.size() == 2);
    const Vector2 a = twins[0];
    const Vector2 b = twins[1];
    auto near = [](Vector2 p, Vector2 q) { return Vector2Distance(p, q) < 0.01f; };
    CAPTURE(a.x, a.y, b.x, b.y);
    CHECK(((near(a, {304.f, 397.f}) && near(b, {296.f, 403.f})) ||
           (near(a, {296.f, 403.f}) && near(b, {304.f, 397.f}))));
}

TEST_CASE("DamageEnemy: multiple hits kills splitter -> only two twins spawned", "[enemy]") {
    World world;
    Enemy &enemy = world.enemies.emplace_back(Vector2{0.f, 0.f}, EnemyType::Splitter);
    enemy.health = 1.f;

    DamageEnemy(world, enemy, 1.f);
    DamageEnemy(world, enemy, 1.f);
    DamageEnemy(world, enemy, 1.f);
    AddNewEnemies(world);

    std::vector<Vector2> twins;
    for (const Enemy &e : world.enemies) {
        if (e.type == EnemyType::Twin) twins.push_back(e.pos);
    }
    CHECK(twins.size() == 2);
}

TEST_CASE("OnSplitterDeath: splitter dead -> vector enemies_to_spawn has 2 twins", "[enemy]") {
    World world;
    Enemy &enemy = world.enemies.emplace_back(world.player.pos, EnemyType::Splitter);

    OnSplitterDeath(world, enemy);

    REQUIRE(world.enemies_to_spawn.size() == 2);
    CHECK(world.enemies_to_spawn[0].type == EnemyType::Twin);
    CHECK(world.enemies_to_spawn[1].type == EnemyType::Twin);
}

TEST_CASE("DamageEnemy: hit enemy to death by multiple weapons -> died once", "[enemy]") {
    World world;
    Enemy &enemy = world.enemies.emplace_back(world.player.pos, EnemyType::Light);
    enemy.health = 1.f;
    Pistol pistol;
    Aura aura;

    pistol.Update(world, Pistol::kFireInterval);
    aura.Update(world, Aura::kFireInterval);

    CHECK(world.exps.size() == 1);
}
