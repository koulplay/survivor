#pragma once

#include "world.h"

[[nodiscard]] std::optional<Vector2> NearestEnemyPos(const World &world);
void DamageEnemy(World &world, Enemy &e, const float damage);
void OnSplitterDeath(World &world, const Enemy& splitter);
void AddNewEnemies(World &world);
void Update(World &world, const Input& input);
bool UpdateLevelUp(World &world, const Input& input);
void ApplyUpgrade(Player &p, Upgrade u);
[[nodiscard]]  bool TryLevelUp(World &world);
void UpdateByState(World &world, const Input &input);
