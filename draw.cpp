#include <algorithm>
#include <cmath>
#include <format>

#include "draw.h"
#include "world.h"


namespace {
[[nodiscard]] float ExpProgress(const World &world) {
    return (world.current_exp / world.exp_to_level);
}

[[nodiscard]] const char *UpgradeName(Upgrade u) {
    // только текст
    switch (u) {
        case Upgrade::FireRate: return TextFormat("Shot Interval -%.1f/s", kBuffFireRate);
        case Upgrade::MoveSpeed: return TextFormat("MoveSpeed +%.1f/s", kBuffMoveSpeed);
        case Upgrade::BulletSpeed: return TextFormat("BulletSpeed +%.1f/s", kBuffBulletSpeed);
        case Upgrade::Magnet: return TextFormat("Magnet +%.1f", kBuffMagnet);
        case Upgrade::Health: return TextFormat("Health +%.1f", kBuffHealth);
        case Upgrade::RestoreHealth: return TextFormat("RestoreHealth +%.1f", kBuffRestoreHealth);
        case Upgrade::Count: break;
    }
    return "?";
}

[[nodiscard]] std::string GameTimeToText(float time) {
    const int total_seconds = static_cast<int>(time);
    const int minutes = total_seconds / 60;
    const int seconds = total_seconds % 60;

    return std::format("{:02}:{:02}", minutes, seconds);
}
}

void Draw(const World &world) {
    BeginDrawing();
    ClearBackground(BLACK);

    const Camera2D camera{
        .offset = {kScreenWidth / 2.f, kScreenHeight / 2.f},
        .target = world.player.pos,
        .rotation = 0.f,
        .zoom = 1.f,
    };

    BeginMode2D(camera);
    constexpr float cell_size = 50.f;
    for (float i = std::floor((world.player.pos.x - kScreenWidth / 2) / cell_size) * cell_size;
         i <= world.player.pos.x + kScreenWidth / 2;
         i += cell_size) {
        DrawLine(i, world.player.pos.y - kScreenHeight / 2, i, world.player.pos.y + kScreenHeight / 2,
                 Fade(GOLD, 0.1f));
    }
    for (float i = std::floor((world.player.pos.y - kScreenHeight / 2) / cell_size) * cell_size;
         i <= world.player.pos.y + kScreenHeight / 2;
         i += cell_size) {
        DrawLine(world.player.pos.x - kScreenWidth / 2, i, world.player.pos.x + kScreenWidth / 2, i, Fade(GOLD, 0.1f));
    }

    DrawRectangleRec(world.player.rect(), RAYWHITE);
    for (const auto &e: world.enemies) {
        const EnemyDef& e_def = GetEnemyDef(e.enemy_type);
        if (e.hit_flash_time > 0.f) {
            DrawCircleV(e.pos, e_def.size, ORANGE);
        }
        else {
            DrawCircleV(e.pos, e_def.size, e_def.color);
        }
    }
    for (const auto &b: world.bullets) {
        DrawCircleV(b.pos, world.player.bullet_radius, YELLOW);
    }
    for (const auto &exp: world.exps) {
        DrawCircleV(exp.pos, kExpRadius, SKYBLUE);
    }

    for (const auto &dp: world.damage_popups) {
        const std::string damage_popup_text = std::format("{:.1f}", dp.damage);
        DrawText(damage_popup_text.c_str(), dp.pos.x - MeasureText(damage_popup_text.c_str(), 15) / 2,
                 dp.pos.y - GetEnemyDef(dp.enemy_type).size - 15, 15, Fade(GetEnemyDef(dp.enemy_type).color, (dp.age / dp.lifetime - 1) * -1));
    }

    EndMode2D();

    const std::string game_time_text = GameTimeToText(world.game_time);
    DrawText(game_time_text.c_str(), (kScreenWidth - MeasureText(game_time_text.c_str(), 20)) / 2, 50, 20, GOLD);

    DrawRectangleLines(kExpBarPos.x, kExpBarPos.y, kExpBarWidth, kExpBarHeight, GOLD);
    DrawRectangle(kExpBarPos.x, kExpBarPos.y,
                  static_cast<int>(std::min(kExpBarWidth * ExpProgress(world), kExpBarWidth)), kExpBarHeight, GOLD);

    const std::string level_text = std::format("LEVEL: {}", world.level);
    DrawText(level_text.c_str(), kExpBarPos.x + kExpBarWidth - MeasureText(level_text.c_str(), 20), kExpBarPos.y + 80,
             20, GOLD);

    if (world.game_state == GameState::LEVELUP) {
        DrawRectangle(0, 0, kScreenWidth, kScreenHeight, Fade(BLACK, 0.6f));
        auto &m = world.lvl_menu;
        for (int i = 0; i < 3; ++i) {
            Rectangle rec = CardRect(i);
            DrawRectangleRec(rec, WHITE);
            if (m.selected == i) {
                DrawRectangleLines(rec.x, rec.y, rec.width, rec.height, GOLD);
            }
            const char *upgrade_name = UpgradeName(m.options[i]);
            DrawText(upgrade_name, rec.x + (rec.width - MeasureText(upgrade_name, 20)) / 2, rec.y + (rec.height / 2),
                     20, GOLD);
        }
    }

    const std::string player_health_text = std::format("HP: {:.0f}", std::ceil(world.player.health));
    DrawText(player_health_text.c_str(),
             (kScreenWidth - MeasureText(player_health_text.c_str(), 20)) / 2, kScreenHeight - 50, 20,
             WHITE);
    if (world.game_state == GameState::GAMEOVER) {
        DrawText("GAME OVER", (kScreenWidth - MeasureText("GAME OVER", 50)) / 2, kScreenHeight / 2, 50, WHITE);
    }

    DrawFPS(10, 10);
    EndDrawing();
}
