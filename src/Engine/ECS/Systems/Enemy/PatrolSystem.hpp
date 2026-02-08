#pragma once
#include "Engine/ECS/Entity.hpp"

namespace GameEngine::ECS {
class Registry;
}
namespace GameEngine::ECS::Systems::PatrolSystem {

void update(Registry &registry);
void updateEntity(Registry &registry, Entity entity);

} // namespace GameEngine::ECS::Systems::PatrolSystem