#pragma once
#include "Engine/ECS/Entity.hpp"
#include "Engine/Utils/SpatialHash.hpp"

namespace GameEngine::ECS {
class Registry;
}

namespace GameEngine::ECS::Systems::CollisionSystem {

Utils::SpatialHash buildStaticColliderGrid(Registry &registry);

void resolveHorizontalCollisions(Entity entity, Registry &registry, const Utils::SpatialHash &grid);

void resolveVerticalCollisions(Entity entity, Registry &registry, const Utils::SpatialHash &grid);

} // namespace GameEngine::ECS::Systems::CollisionSystem
