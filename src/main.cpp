#include <raylib.h>

#include "world.h"
#include "update.h"
#include "draw.h"

void ReadInput(Input &input) {
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) input.player_move_up = true;
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) input.player_move_left = true;
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) input.player_move_down = true;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) input.player_move_right = true;
    if (IsKeyPressed(KEY_R)) input.restart = true;
    if (IsKeyPressed(KEY_LEFT)) input.card_left = true;
    if (IsKeyPressed(KEY_RIGHT)) input.card_right = true;
    if (IsKeyPressed(KEY_ENTER)) input.card_choose = true;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) input.card_choose_mouse = true;
    if (IsKeyPressed(KEY_ONE)) input.picked_card = 0;
    if (IsKeyPressed(KEY_TWO)) input.picked_card = 1;
    if (IsKeyPressed(KEY_THREE)) input.picked_card = 2;
    input.mouse_pos = GetMousePosition();
    input.mouse_delta = GetMouseDelta();
}

int main() {
    InitWindow(kScreenWidth, kScreenHeight, "Survivor");
    SetTargetFPS(60);

    World world;
    Input input;
    while (!WindowShouldClose()) {
        input = Input{};
        ReadInput(input);
        switch (world.game_state) {
            case GameState::Playing:
                Update(world, input, std::min(GetFrameTime(), 0.1f));
                break;
            case GameState::LevelUp:
                if (UpdateLevelUp(world, input)) {
                    ApplyUpgrade(world.player, world.level_up_menu.options[world.level_up_menu.selected]);
                    world.game_state = GameState::Playing;
                }
                break;
            case GameState::GameOver:
                if (input.restart) {
                    world = World{};
                }
                break;
        }
        Draw(world);
    }

    CloseWindow();
    return 0;
}
