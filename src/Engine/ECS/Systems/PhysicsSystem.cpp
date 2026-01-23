#include "PhysicsSystem.hpp"

#include "CollisionSystem.hpp"
#include "Engine/Debug/ScopedProfiler.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Hazard/LastGroundedPositionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/AccelerationComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Physics/GravityComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Tags/InactiveTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"

#include <glm/glm.hpp>

namespace GameEngine::ECS::Systems::PhysicsSystem {

static Utils::SpatialHash staticGrid(128.0f);

void initStaticCollision(Registry &registry)
{
	staticGrid = CollisionSystem::buildStaticColliderGrid(registry);
}
void update(Registry &registry, const float timeStep)
{
	PROFILE_SCOPE("PhysicsSystem");

	for (auto viewGravity = registry.view<VelocityComponent, GravityComponent>().exclude<InactiveTag>();
	     const auto &entity : viewGravity) {
		auto &[velocity] = viewGravity.get<VelocityComponent>(entity);
		const auto &[gravity] = viewGravity.get<GravityComponent>(entity);
		velocity += glm::vec2(0, gravity) * timeStep;
	}

	for (auto view = registry.view<MovementComponent, PositionComponent, VelocityComponent, AccelerationComponent>()
	                     .exclude<InactiveTag>();
	     const auto entity : view) {
		auto &pos = view.get<PositionComponent>(entity);
		auto &vel = view.get<VelocityComponent>(entity);
		auto &acc = view.get<AccelerationComponent>(entity);
		const auto &[accelerationRate, maxSpeedX] = view.get<MovementComponent>(entity);

		vel.velocity += acc.acceleration * timeStep;
		if (std::abs(vel.velocity.x) > maxSpeedX) {
			vel.velocity.x = maxSpeedX * (vel.velocity.x > 0 ? 1 : -1);
		}

		auto collisionComponent = registry.tryGet<CollisionComponent>(entity);

		if (collisionComponent) {
			const auto transform = registry.tryGet<TransformComponent>(entity);

			CollisionSystem::EntityPhysicsState state{
			    entity, pos, vel, acc, collisionComponent->get(), transform ? &transform->get() : nullptr};

			pos.position.x += vel.velocity.x * timeStep;
			CollisionSystem::resolveHorizontalCollisions(state, staticGrid);

			pos.position.y += vel.velocity.y * timeStep;
			CollisionSystem::resolveVerticalCollisions(state, staticGrid);

			if (const auto groundedComp = registry.tryGet<GroundedComponent>(entity)) {
				const auto lastGroundedComp = registry.tryGet<LastGroundedPositionComponent>(entity);

				CollisionSystem::updateGroundedState(state, groundedComp->get(),
				                                     lastGroundedComp ? &lastGroundedComp->get() : nullptr, staticGrid);
			}
		} else {
			pos.position.x += vel.velocity.x * timeStep;
			pos.position.y += vel.velocity.y * timeStep;
		}

		pos.previousPosition = pos.position;
	};
}

} // namespace GameEngine::ECS::Systems::PhysicsSystem
