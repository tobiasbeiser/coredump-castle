#pragma once
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/Utils/SpatialHash.hpp"
#include <glm/vec2.hpp>

struct PositionComponent;
struct VelocityComponent;
struct AccelerationComponent;
struct CollisionComponent;
struct GroundedComponent;
struct LastGroundedPositionComponent;

namespace GameEngine::ECS::Systems::CollisionSystem {

// NOLINTBEGIN
struct EntityPhysicsState {
	Entity entity;
	PositionComponent &pos;
	VelocityComponent &vel;
	AccelerationComponent &acc;
	const CollisionComponent &col;
	const TransformComponent *transform = nullptr;
};
// NOLINTEND

Utils::SpatialHash buildStaticColliderGrid(Registry &registry);

void resolveHorizontalCollisions(const EntityPhysicsState &state, const Utils::SpatialHash &grid);
void resolveVerticalCollisions(const EntityPhysicsState &state, const Utils::SpatialHash &grid);

void updateGroundedState(const EntityPhysicsState &state, GroundedComponent &groundedComp,
                         LastGroundedPositionComponent *lastGroundedComp, const Utils::SpatialHash &grid);

bool isPathObstructed(const glm::vec2 &start, const glm::vec2 &end, const Utils::SpatialHash &grid, const glm::vec2 &projectileSize = {0.0f, 0.0f});

} // namespace GameEngine::ECS::Systems::CollisionSystem