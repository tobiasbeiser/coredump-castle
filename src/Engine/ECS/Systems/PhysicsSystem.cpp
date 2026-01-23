#include "PhysicsSystem.hpp"

#include "CollisionSystem.hpp"
#include "Engine/Debug/ScopedProfiler.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/AccelerationComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Physics/GravityComponent.hpp"
#include "Engine/ECS/Components/Tags/InactiveTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/ECS/Systems/Enemy/ProjectileSystem.hpp"
#include <glm/glm.hpp>

namespace GameEngine::ECS::Systems::PhysicsSystem {

void update(Registry &registry, const float timeStep)
{
	PROFILE_SCOPE("PhysicsSystem");

	const auto staticColliderGrid = CollisionSystem::buildStaticColliderGrid(registry);

	for (auto viewGravity = registry.view<VelocityComponent, GravityComponent>().exclude<InactiveTag>();
	     const auto &entity : viewGravity) {
		auto &[velocity] = viewGravity.get<VelocityComponent>(entity);
		const auto &[gravity] = viewGravity.get<GravityComponent>(entity);
		velocity += glm::vec2(0, gravity) * timeStep;
	}

	for (auto view = registry.view<MovementComponent, PositionComponent, VelocityComponent, AccelerationComponent>()
	                     .exclude<InactiveTag>();
	     const auto entity : view) {
		auto &[position, previousPosition] = view.get<PositionComponent>(entity);
		auto &[velocity] = view.get<VelocityComponent>(entity);
		auto &[acceleration] = view.get<AccelerationComponent>(entity);
		const auto &[accelerationRate, maxSpeedX] = view.get<MovementComponent>(entity);

		velocity += acceleration * timeStep;
		if (std::abs(velocity.x) > maxSpeedX) {
			velocity.x = maxSpeedX * (velocity.x > 0 ? 1 : -1);
		}
		position.x += velocity.x * timeStep;
		if (registry.has<CollisionComponent>(entity)) {
			CollisionSystem::resolveHorizontalCollisions(entity, registry, staticColliderGrid);
		}
		position.y += velocity.y * timeStep;
		if (registry.has<CollisionComponent>(entity)) {
			CollisionSystem::resolveVerticalCollisions(entity, registry, staticColliderGrid);
		}

		previousPosition = position;
	};

	ProjectileSystem::handleEnemyProjectileCollisions(registry, staticColliderGrid);
}

} // namespace GameEngine::ECS::Systems::PhysicsSystem
