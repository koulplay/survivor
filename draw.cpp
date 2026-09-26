#include <algorithm>

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
    EndMode2D();


    DrawRectangleLines(kExpBarPos.x, kExpBarPos.y, kExpBarWidth, kExpBarHeight, GOLD);
    DrawRectangle(kExpBarPos.x, kExpBarPos.y,
                  static_cast<int>(std::min(kExpBarWidth * ExpProgress(world), kExpBarWidth)), kExpBarHeight, GOLD);
    const char *level_text = TextFormat("LEVEL: %d", world.level);
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
            const char *upgrade_name = UpgradeName(m.options[i]);
            DrawText(upgrade_name, rec.x + (rec.width - MeasureText(upgrade_name, 20)) / 2, rec.y + (rec.height / 2),
                     20, GOLD);
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

