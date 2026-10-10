#pragma once

#include "world.h"

void Update(World &world, const float dt);
bool UpdateLevelUp(World &world);
void ApplyUpgrade(Player &p, Upgrade u);
void UpdateLevelUpExperience(World &world);
