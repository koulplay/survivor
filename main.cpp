#include <optional>
#include <raylib.h>
#include <raymath.h>
#include <vector>
#include <array>
#include <algorithm>
#include <random>


constexpr int kScreenWidth = 1280;
constexpr int kScreenHeight = 720;

constexpr float kEnemyRadius = 10.0f;
constexpr float kEnemySpeed = 50.f;
constexpr float kEnemyDamage = 30.f;

constexpr float kExpRadius = 2.0f;
constexpr float kExpSpeed = 300.f;
constexpr float kExpValue = 5.f;

constexpr float kExpBarWidth = 300.f;
constexpr float kExpBarHeight = 50.f;
constexpr Vector2 kExpBarPos = {kScreenWidth - 320.f, 20.f};

constexpr float kCardWidth = 300.f;
constexpr float kCardHeight = 500.f;
constexpr float kCardGap = 50.f;

constexpr float kBuffFireRate = 0.2f;
constexpr float kBuffMoveSpeed = 10.f;
constexpr float kBuffBulletSpeed = 25.f;
constexpr float kBuffMagnet = 5.f;
constexpr float kBuffHealth = 5.f;
constexpr float kBuffRestoreHealth = 25.f;


enum class GameState {
    PLAYING,
    LEVELUP,
    GAMEOVER
};

enum class Upgrade { FireRate, MoveSpeed, BulletSpeed, Magnet, Health, RestoreHealth, Count };

struct LevelUpMenu {
    std::array<Upgrade, 3> options{};
    int selected = 0;
};

Rectangle CardRect(int i) {
    // одна функция и для рисования, и для мыши
    const float total = 3 * kCardWidth + 2 * kCardGap;
    return {
        (kScreenWidth - total) / 2 + i * (kCardWidth + kCardGap), (kScreenHeight - kCardHeight) / 2, kCardWidth,
        kCardHeight
    };
}

struct Exp {
    Vector2 pos;
    bool dead = false;
};

struct Bullet {
    Vector2 pos;
    Vector2 vel;
    bool dead = false;
};

struct Player {
    float size = 32.0f;
    float speed = 150.f;
    float fire_rate = 2.f;
    float bullet_radius = 3.0f;
    float bullet_speed = 300.f;

    Vector2 pos{kScreenWidth / 2.f, kScreenHeight / 2.f};
    float health = 100.f;
    float max_health = 100.f;
    float bullet_spawn_time = 0.f;
    float magnet_radius = 60.f;

    [[nodiscard]] Rectangle rect() const {
        return {pos.x - size / 2, pos.y - size / 2, size, size};
    }
};

struct Enemy {
    Vector2 pos;
    bool dead = false;
};

struct World {
    Player player;
    std::vector<Enemy> enemies;
    std::vector<Bullet> bullets;
    std::vector<Exp> exps;

    LevelUpMenu lvl_menu;
    GameState game_state = GameState::PLAYING;
    float current_exp = 0.f;
    float exp_to_level = 1.f;
    int level = 0;
    float enemy_spawn_time = 0.f;
};

[[nodiscard]] Vector2 RandomEdgePoint() {
    const float x = static_cast<float>(GetRandomValue(0, kScreenWidth));
    const float y = static_cast<float>(GetRandomValue(0, kScreenHeight));
    switch (GetRandomValue(0, 3)) {
        case 0: return {x, 0.0f};
        case 1: return {x, kScreenHeight};
        case 2: return {0.0f, y};
        default: return {kScreenWidth, y};
    }
}

[[nodiscard]] Vector2 ReadMoveInput() {
    Vector2 move_direction{0.0f, 0.0f};

    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) {
        move_direction += {0.0f, -1.f};
    }
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
        move_direction += {-1.f, 0.0f};
    }
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) {
        move_direction += {0.0f, 1.f};
    }
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
        move_direction += {1.f, 0.0f};
    }
    return Vector2Normalize(move_direction);
}

void UpdateSpawn(World &world, const float dt) {
    world.enemy_spawn_time += dt;
    if (world.enemy_spawn_time > 1.0f) {
        world.enemy_spawn_time -= 1.0f;
        world.enemies.emplace_back(RandomEdgePoint());
    }
}

void UpdatePlayer(Player &player, const float dt) {
    player.pos += ReadMoveInput() * (player.speed * dt);
}

void UpdateEnemies(World &world, const float dt) {
    for (auto &e: world.enemies) {
        e.pos = Vector2MoveTowards(e.pos, world.player.pos, kEnemySpeed * dt);
        if (CheckCollisionCircleRec(e.pos, kEnemyRadius, world.player.rect())) {
            world.player.health -= kEnemyDamage * dt;
        }
    }
}

std::optional<Vector2> NearestEnemyPos(const World &world) {
    if (world.enemies.empty()) {
        return std::nullopt;
    }

    float distance_to_enemy = Vector2Distance(world.player.pos, world.enemies[0].pos);
    Vector2 enemy_pos = world.enemies[0].pos;
    for (auto &e: world.enemies) {
        const float new_distance = Vector2Distance(world.player.pos, e.pos);
        if (new_distance < distance_to_enemy) {
            distance_to_enemy = new_distance;
            enemy_pos = e.pos;
        }
    }
    return enemy_pos;
}

// таймер + новая пуля в ближайшего
void UpdateShooting(World &world, const float dt) {
    Player &p = world.player;
    p.bullet_spawn_time += dt;
    if (p.bullet_spawn_time < p.fire_rate) return;

    const std::optional<Vector2> target = NearestEnemyPos(world);
    if (!target) { p.bullet_spawn_time = p.fire_rate; return; }

    p.bullet_spawn_time -= p.fire_rate;
    world.bullets.push_back({p.pos, Vector2Normalize(*target - p.pos) * p.bullet_speed});
}

// полёт, попадания, удаление
void UpdateBullets(World &world, const float dt) {
    for (auto &b: world.bullets) {
        if (b.pos.x > kScreenWidth || b.pos.x < 0 || b.pos.y > kScreenHeight || b.pos.y < 0) {
            b.dead = true;
            continue;
        }
        if (b.dead) {
            continue;
        }
        for (auto &e: world.enemies) {
            if (e.dead) {
                continue;
            }
            if (CheckCollisionCircles(b.pos, world.player.bullet_radius, e.pos, kEnemyRadius)) {
                b.dead = true;
                e.dead = true;

                world.exps.push_back({e.pos});
                break;
            }
        }
        b.pos += b.vel * dt;
    }
}

void UpdateExp(World &world, const float dt) {
    for (auto &exp: world.exps) {
        if (CheckCollisionCircles(world.player.pos, world.player.magnet_radius, exp.pos, kExpRadius)) {
            exp.pos = Vector2MoveTowards(exp.pos, world.player.pos, kExpSpeed * dt);
        }
        if (CheckCollisionCircleRec(exp.pos, kExpRadius, world.player.rect())) {
            world.current_exp += kExpValue;
            exp.dead = true;
        }
    }
}

void UpdateGameState(World &world) {
    if (world.player.health <= 0.f && world.game_state != GameState::GAMEOVER) {
        world.player.health = 0.f;
        world.game_state = GameState::GAMEOVER;
    }
    else if (world.current_exp >= world.exp_to_level && world.game_state != GameState::LEVELUP) {
        static std::mt19937 rng{std::random_device{}()};
        std::array<Upgrade, static_cast<int>(Upgrade::Count)> all{};
        for (int i = 0; i < static_cast<int>(all.size()); ++i) all[i] = static_cast<Upgrade>(i);
        std::shuffle(all.begin(), all.end(), rng);
        for (int i = 0; i < 3; ++i) world.lvl_menu.options[i] = all[i];
        world.lvl_menu.selected = 0;

        ++world.level;
        const float mod = world.current_exp - world.exp_to_level;
        world.current_exp = mod;
        world.exp_to_level *= 1.2f;
        world.game_state = GameState::LEVELUP;
    }
    else {
        world.game_state = GameState::PLAYING;
    }
}

void Update(World &world, const float dt) {
    UpdateSpawn(world, dt);
    UpdatePlayer(world.player, dt);
    UpdateEnemies(world, dt);
    UpdateShooting(world, dt);
    UpdateBullets(world, dt);
    UpdateExp(world, dt);

    UpdateGameState(world);

    std::erase_if(world.enemies, [](const Enemy &e) { return e.dead; });
    std::erase_if(world.bullets, [](const Bullet &b) { return b.dead; });
    std::erase_if(world.exps, [](const Exp &e) { return e.dead; });
}

[[nodiscard]] float ExpProgress(const World &world) {
    return (world.current_exp / world.exp_to_level);
}

[[nodiscard]] const char* UpgradeName(Upgrade u) {  // только текст
    switch (u) {
        case Upgrade::FireRate: return TextFormat("Shot Interval -%.1f/s", kBuffFireRate);
        case Upgrade::MoveSpeed: return TextFormat("MoveSpeed +%.1f/s", kBuffMoveSpeed);
        case Upgrade::BulletSpeed: return TextFormat("BulletSpeed +%.1f/s", kBuffBulletSpeed);
        case Upgrade::Magnet: return TextFormat("Magnet +%.1f", kBuffMagnet);
        case Upgrade::Health: return TextFormat("Health +%.1f", kBuffHealth);
        case Upgrade::RestoreHealth: return TextFormat("RestoreHealth +%.1f", kBuffRestoreHealth);
        case Upgrade::Count:     break;
    }
    return "?";
}

void ApplyUpgrade(Player& p, Upgrade u) {  // только изменение
    switch (u) {
        case Upgrade::FireRate: p.fire_rate = std::max(0.1f, p.fire_rate - kBuffFireRate); break;
        case Upgrade::MoveSpeed: p.speed += kBuffMoveSpeed; break;
        case Upgrade::BulletSpeed: p.bullet_speed += kBuffBulletSpeed; break;
        case Upgrade::Magnet: p.magnet_radius += kBuffMagnet; break;
        case Upgrade::Health: p.max_health += kBuffHealth; p.health += kBuffHealth; break;
        case Upgrade::RestoreHealth: p.health = std::min(p.health + kBuffRestoreHealth, p.max_health); break;
        case Upgrade::Count:     break;
    }
}

void Draw(const World &world) {
    BeginDrawing();
    ClearBackground(BLACK);

    DrawRectangleRec(world.player.rect(), RAYWHITE);

    for (const auto &e: world.enemies) {
        DrawCircleV(e.pos, kEnemyRadius, RED);
    }

    for (const auto &b: world.bullets) {
        DrawCircleV(b.pos, world.player.bullet_radius, YELLOW);
    }

    for (const auto &exp: world.exps) {
        DrawCircleV(exp.pos, kExpRadius, SKYBLUE);
    }

    DrawRectangleLines(kExpBarPos.x, kExpBarPos.y, kExpBarWidth, kExpBarHeight, GOLD);
    DrawRectangle(kExpBarPos.x, kExpBarPos.y, static_cast<int>(std::min(kExpBarWidth * ExpProgress(world), kExpBarWidth)), kExpBarHeight, GOLD);
    const char* level_text = TextFormat("LEVEL: %d", world.level);
    DrawText(level_text, kExpBarPos.x + kExpBarWidth - MeasureText(level_text, 20), kExpBarPos.y + 80, 20, GOLD);

    if (world.game_state == GameState::LEVELUP) {
        DrawRectangle(0, 0, kScreenWidth, kScreenHeight, Fade(BLACK, 0.6f));
        auto &m = world.lvl_menu;
        for (int i = 0; i < 3; ++i) {
            Rectangle rec = CardRect(i);
            DrawRectangleRec(rec, WHITE);
            if (m.selected == i) {
                DrawRectangleLines(rec.x, rec.y, rec.width, rec.height, GOLD);
            }
            const char* upgrade_name = UpgradeName(m.options[i]);
            DrawText(upgrade_name, rec.x + (rec.width - MeasureText(upgrade_name, 20))/ 2, rec.y + (rec.height / 2), 20, GOLD);
        }
    }

    DrawText(TextFormat("HP: %.0f", world.player.health),
             (kScreenWidth - MeasureText(TextFormat("HP: %.0f", world.player.health), 20)) / 2, kScreenHeight - 50, 20,
             WHITE);
    if (world.game_state == GameState::GAMEOVER) {
        DrawText("GAME OVER", (kScreenWidth - MeasureText("GAME OVER", 50)) / 2, kScreenHeight / 2, 50, WHITE);
    }

    DrawFPS(10, 10);
    EndDrawing();
}

bool UpdateLevelUp(World &world) {
    auto &m = world.lvl_menu;
    if (IsKeyPressed(KEY_RIGHT)) m.selected = (m.selected + 1) % 3;
    if (IsKeyPressed(KEY_LEFT)) m.selected = (m.selected + 3 - 1) % 3;
    const bool mouse_moved = Vector2Length(GetMouseDelta()) > 0.0f;
    bool confirm = IsKeyPressed(KEY_ENTER);
    for (int i = 0; i < 3; ++i) {
        if (IsKeyPressed(KEY_ONE + i)) {
            m.selected = i;
            confirm = true;
        }
        if (CheckCollisionPointRec(GetMousePosition(), CardRect(i))) {
            if (mouse_moved) m.selected = i;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) confirm = true;
        }
    }
    return confirm;
}

int main() {
    InitWindow(kScreenWidth, kScreenHeight, "Survivor");
    SetTargetFPS(60);

    World world;
    while (!WindowShouldClose()) {
        switch (world.game_state) {
            case GameState::PLAYING:
                Update(world, GetFrameTime());
                break;
            case GameState::LEVELUP:
                if (UpdateLevelUp(world)) {
                    ApplyUpgrade(world.player, world.lvl_menu.options[world.lvl_menu.selected]);
                    world.game_state = GameState::PLAYING;
                }
                    break;
            case GameState::GAMEOVER:
                if (IsKeyPressed(KEY_R)) {
                    world = World{};
                }
                break;
        }
        Draw(world);
    }

    CloseWindow();
    return 0;
}
