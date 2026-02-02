#pragma once
#include "Engine/ECS/Entity.hpp"

namespace GameEngine::ECS {
class Registry;
}

namespace GameEngine::ECS::Systems::ChaseSystem {

void update(Registry &registry, float deltaTime);
void updateEntity(Registry &registry, Entity entity, float deltaTime);

} // namespace GameEngine::ECS::Systems::ChaseSystem