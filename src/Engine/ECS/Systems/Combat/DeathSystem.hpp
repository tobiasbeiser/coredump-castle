#pragma once
#include "Engine/ECS/Registry.hpp"

namespace GameEngine::ECS::Systems::DeathSystem {

void update(Registry &registry, float deltaTime);

} // namespace GameEngine::ECS::Systems::DeathSystem