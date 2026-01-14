#pragma once
#include "Engine/ECS/Entity.hpp"
#include "Engine/Utils/SpatialHash.hpp"
#include <glm/vec2.hpp>

namespace GameEngine::ECS {
class Registry;
}

namespace GameEngine::ECS::Systems::CollisionSystem {

Utils::SpatialHash buildStaticColliderGrid(Registry &registry);

void resolveHorizontalCollisions(Entity entity, Registry &registry, const Utils::SpatialHash &grid);

void resolveVerticalCollisions(Entity entity, Registry &registry, const Utils::SpatialHash &grid);

void handleEnemyProjectileCollisions(Registry &registry, const Utils::SpatialHash &grid);

bool isPathObstructed(const glm::vec2 &start, const glm::vec2 &end, const Utils::SpatialHash &grid, const glm::vec2 &projectileSize = {0.0f, 0.0f});

} // namespace GameEngine::ECS::Systems::CollisionSystem
