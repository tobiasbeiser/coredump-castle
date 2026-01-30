#pragma once
#include "Engine/GameEngine.hpp"
#include "Game/Spawners/EnemySpawner.hpp"
namespace GameEngine::ECS {
class Registry;
}

namespace GameEngine::ECS::Systems::InteractionSystem {

void update(GameEngine &game, Registry &registry, std::vector<EnemySpawner> &spawners);

} // namespace GameEngine::ECS::Systems::InteractionSystem