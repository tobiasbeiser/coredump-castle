#pragma once
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/Utils/SpatialHash.hpp"

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

} // namespace GameEngine::ECS::Systems::CollisionSystem