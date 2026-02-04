#pragma once
#include "Engine/ECS/Registry.hpp"

namespace GameEngine::ECS::Systems::ChargeSystem {
void update(Registry &registry, float deltaTime);
void updateEntity(Registry &registry, Entity entity, float deltaTime);
}
