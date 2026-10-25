#pragma once

#include "world.h"

void UpdateBullets(World &world, const float dt);
void UpdateAura(World &world, const float dt);
void OnSplitterDeath(World &world, const Enemy& splitter);
void UpdatePlayerDamage(World &world);
void Update(World &world, const Input& input);
bool UpdateLevelUp(World &world, const Input& input);
void ApplyUpgrade(Player &p, Upgrade u);
[[nodiscard]]  bool TryLevelUp(World &world);
void UpdateByState(World &world, const Input &input);
