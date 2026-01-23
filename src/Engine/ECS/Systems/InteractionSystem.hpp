#pragma once
#include "Engine/GameEngine.hpp"
namespace GameEngine::ECS {
class Registry;
}

namespace GameEngine::ECS::Systems::InteractionSystem {

void update(GameEngine &game, Registry &registry);

} // namespace GameEngine::ECS::Systems::InteractionSystem