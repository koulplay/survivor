#include "world.h"
#include "update.h"
#include "draw.h"

int main() {
    InitWindow(kScreenWidth, kScreenHeight, "Survivor");
    SetTargetFPS(60);

    World world;
    while (!WindowShouldClose()) {
        switch (world.game_state) {
            case GameState::PLAYING:
                Update(world, std::min(GetFrameTime(), 0.1f));
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
