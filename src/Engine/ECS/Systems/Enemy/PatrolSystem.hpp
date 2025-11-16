#pragma once
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Registry.hpp"

namespace GameEngine::ECS::Systems {
struct PatrolSystem {
	static void update(Registry &registry, const float stepsize)
	{
		auto view = registry.view<EnemyTag, PositionComponent, VelocityComponent, PatrolComponent, RenderComponent,
		                          CollisionComponent>();
		for (auto entity : view) {
			auto &pos = registry.get<PositionComponent>(entity);
			auto &vel = registry.get<VelocityComponent>(entity);
			auto &patrol = registry.get<PatrolComponent>(entity);
			auto &render = registry.get<RenderComponent>(entity);
			const auto &collision = registry.get<CollisionComponent>(entity);

			float enemyRight = pos.position.x + collision.collisionBox.x + collision.collisionBox.w;
			float enemyLeft = pos.position.x + collision.collisionBox.x;

			vel.velocity.x = patrol.speed * patrol.direction;

			if (patrol.direction > 0 && enemyRight >= patrol.maxBounds.x) {
				patrol.direction = -1;
				render.flipX = false;
				pos.position.x = patrol.maxBounds.x - (enemyRight - pos.position.x);

			} else if (patrol.direction < 0 && enemyLeft <= patrol.minBounds.x) {
				patrol.direction = 1;
				render.flipX = true;
				pos.position.x = patrol.minBounds.x - (enemyLeft - pos.position.x);
			}
		}
	}
};

} // namespace GameEngine::ECS::Systems