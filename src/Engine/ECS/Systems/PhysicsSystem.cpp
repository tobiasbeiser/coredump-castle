#include "PhysicsSystem.hpp"

#include "CollisionSystem.hpp"
#include "Engine/Debug/DebugSystem.hpp"
#include "Engine/Debug/ScopedProfiler.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Hazard/LastGroundedPositionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/AccelerationComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Physics/GravityComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Components/Tags/InactiveTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/Enemy/ProjectileSystem.hpp"
#include "Engine/ECS/View.hpp"

#include <glm/glm.hpp>

namespace GameEngine::ECS::Systems::PhysicsSystem {

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

			bool ignoreLocks = false;
			if (registry.has<PlayerTag>(entity) && DebugSystem::instance().getCheats().noClip) {
				ignoreLocks = true;
			}

			CollisionSystem::EntityPhysicsState state{
			    entity, pos, vel, acc, collisionComponent->get(), transform ? &transform->get() : nullptr, ignoreLocks};

			pos.position.x += vel.velocity.x * timeStep;
			CollisionSystem::resolveHorizontalCollisions(state);

			pos.position.y += vel.velocity.y * timeStep;
			CollisionSystem::resolveVerticalCollisions(state);

			if (const auto groundedComp = registry.tryGet<GroundedComponent>(entity)) {
				const auto lastGroundedComp = registry.tryGet<LastGroundedPositionComponent>(entity);

				CollisionSystem::updateGroundedState(state, groundedComp->get(),
				                                     lastGroundedComp ? &lastGroundedComp->get() : nullptr);
			}
		} else {
			pos.position.x += vel.velocity.x * timeStep;
			pos.position.y += vel.velocity.y * timeStep;
		}

		pos.previousPosition = pos.position;
	};

	ProjectileSystem::handleEnemyProjectileCollisions(registry);
}

} // namespace GameEngine::ECS::Systems::PhysicsSystem
