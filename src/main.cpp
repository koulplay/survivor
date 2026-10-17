#include <raylib.h>

#include "world.h"
#include "update.h"
#include "draw.h"

void ReadInput(Input &input) {
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) input.down.player_move_up = true;
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) input.down.player_move_left = true;
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) input.down.player_move_down = true;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) input.down.player_move_right = true;
    if (IsKeyPressed(KEY_R)) input.pressed.restart = true;
    if (IsKeyPressed(KEY_LEFT)) input.pressed.card_left = true;
    if (IsKeyPressed(KEY_RIGHT)) input.pressed.card_right = true;
    if (IsKeyPressed(KEY_ENTER)) input.pressed.card_choose = true;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) input.pressed.card_choose_mouse = true;
    if (IsKeyPressed(KEY_ONE)) input.pressed.picked_card = 0;
    if (IsKeyPressed(KEY_TWO)) input.pressed.picked_card = 1;
    if (IsKeyPressed(KEY_THREE)) input.pressed.picked_card = 2;
    input.pressed.mouse_pos = GetMousePosition();
    input.pressed.mouse_delta = GetMouseDelta();
}

int main() {
    InitWindow(kScreenWidth, kScreenHeight, "Survivor");
    SetTargetFPS(144);

    World world;
    Input input;
    double accumulator = 0.f;

    while (!WindowShouldClose()) {
        ReadInput(input);
        accumulator += std::min(GetFrameTime(), kMaxFrame);
        while (accumulator >= kTick) {
            switch (world.game_state) {
                case GameState::Playing:
                    Update(world, input);
                    break;
                case GameState::LevelUp:
                    if (UpdateLevelUp(world, input)) {
                        ApplyUpgrade(world.player, world.level_up_menu.options[world.level_up_menu.selected]);
                        world.game_state = GameState::Playing;
                    }
                    break;
                case GameState::GameOver:
                    if (input.pressed.restart) {
                        world = World{};
                    }
                    break;
            }
            input.pressed = Input::Pressed{};
            accumulator -= kTick;
        }
        input.down = Input::Down{};
        Draw(world);
    }

    CloseWindow();
    return 0;
}
