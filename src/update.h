#pragma once

#include "world.h"

void Update(World &world, const Input& input, const float dt);
bool UpdateLevelUp(World &world, const Input& input);
void ApplyUpgrade(Player &p, Upgrade u);
[[nodiscard]]  bool TryLevelUp(World &world);
