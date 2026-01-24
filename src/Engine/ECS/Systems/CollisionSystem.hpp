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
	bool ignoreLocks = false;
};
// NOLINTEND

void buildStaticColliderGrid(Registry &registry);

void resolveHorizontalCollisions(const EntityPhysicsState &state);
void resolveVerticalCollisions(const EntityPhysicsState &state);

void updateGroundedState(const EntityPhysicsState &state, GroundedComponent &groundedComp,
                         LastGroundedPositionComponent *lastGroundedComp);

bool isPathObstructed(const glm::vec2 &start, const glm::vec2 &end, const glm::vec2 &projectileSize = {0.0f, 0.0f});

} // namespace GameEngine::ECS::Systems::CollisionSystem