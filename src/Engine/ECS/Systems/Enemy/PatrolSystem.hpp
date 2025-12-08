#pragma once
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"

namespace GameEngine::ECS::Systems {
struct PatrolSystem {
	static void update(Registry &registry)
	{
		auto view = registry.view<EnemyTag, PositionComponent, VelocityComponent, PatrolComponent, RenderComponent,
		                          CollisionComponent>();

		for (const auto entity : view) {
			auto &[position, previousPosition] = registry.get<PositionComponent>(entity);
			auto &[velocity] = registry.get<VelocityComponent>(entity);
			auto &[minBounds, maxBounds, speed, direction] = registry.get<PatrolComponent>(entity);
			auto &render = registry.get<RenderComponent>(entity);
			const auto &[collisionBox] = registry.get<CollisionComponent>(entity);

			const float enemyRight = position.x + collisionBox.x + collisionBox.w;
			const float enemyLeft = position.x + collisionBox.x;

			velocity.x = speed * direction;

			if (direction > 0 && enemyRight >= maxBounds.x) {
				direction = -1;
				render.flipX = false;
				position.x = maxBounds.x - (enemyRight - position.x);

			} else if (direction < 0 && enemyLeft <= minBounds.x) {
				direction = 1;
				render.flipX = true;
				position.x = minBounds.x - (enemyLeft - position.x);
			}
		}
	}
};
} // namespace GameEngine::ECS::Systems