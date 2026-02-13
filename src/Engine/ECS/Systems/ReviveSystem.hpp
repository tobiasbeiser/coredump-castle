#pragma once

#include "Engine/ECS/Registry.hpp"
#include "Engine/GameEngine.hpp"
#include "Game/Spawners/EnemySpawner.hpp"

namespace GameEngine::ECS::Systems::ReviveSystem{

void update(Registry& registry, GameEngine& game, std::vector<EnemySpawner>& spawners);

} // namespace GameEngine::ECS::Systems::RespawnSystem
