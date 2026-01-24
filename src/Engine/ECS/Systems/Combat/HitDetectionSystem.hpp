#pragma once
#include "Engine/ECS/Entity.hpp"

#include <functional>

namespace GameEngine::ECS {
class Registry;
}

namespace GameEngine::ECS::Systems::HitDetectionSystem {
using EnemyHitCallback = std::function<void(Entity, Entity)>;

void update(Registry &registry, const EnemyHitCallback &onHit);

} // namespace GameEngine::ECS::Systems::HitDetectionSystem