#pragma once
#include "Engine/GameEngine.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Game/Spawners/EnemySpawner.hpp"

namespace GameEngine::ECS::Systems::DeathSystem {

void update(Registry& registry, GameEngine& game, float deltaTime, std::vector<EnemySpawner> &spawners);

} // namespace GameEngine::ECS::Systems::DeathSystem