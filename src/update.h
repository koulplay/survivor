#pragma once

#include "world.h"

void UpdateShooting(World &world, const float dt);
void UpdateBullets(World &world, const float dt);
void Update(World &world, const Input& input);
bool UpdateLevelUp(World &world, const Input& input);
void ApplyUpgrade(Player &p, Upgrade u);
[[nodiscard]]  bool TryLevelUp(World &world);
