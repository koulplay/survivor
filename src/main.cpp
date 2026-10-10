#include "world.h"
#include "update.h"
#include "draw.h"

int main() {
    InitWindow(kScreenWidth, kScreenHeight, "Survivor");
    SetTargetFPS(60);

    World world;
    while (!WindowShouldClose()) {
        switch (world.game_state) {
            case GameState::Playing:
                Update(world, std::min(GetFrameTime(), 0.1f));
                break;
            case GameState::LevelUp:
                if (UpdateLevelUp(world)) {
                    ApplyUpgrade(world.player, world.level_up_menu.options[world.level_up_menu.selected]);
                    world.game_state = GameState::Playing;
                }
                break;
            case GameState::GameOver:
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
