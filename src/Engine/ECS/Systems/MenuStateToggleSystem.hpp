#pragma once
#include "Engine/GameEngine.hpp"

namespace GameEngine::ECS {
class Registry;
}

namespace GameEngine::ECS::Systems::MenuStateToggleSystem {

void update(Registry &registry, GameEngine &game);

} // namespace GameEngine::ECS::Systems::MapToggleSystem