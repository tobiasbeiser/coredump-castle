#pragma once
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"

#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"

namespace GameEngine::ECS::Systems {
struct PatrolSystem {
	static void update(Registry &registry)
	{

		for (auto view = registry.view<PositionComponent, PatrolComponent, InputComponent, CollisionComponent>();
		     const auto entity : view) {
			if (registry.has<ChaseComponent>(entity)) {
				const auto &chase = registry.get<ChaseComponent>(entity);
				if (chase.lastKnownPlayerX.has_value()) {
					continue;
				}
			}
			if (registry.has<AttackComponent>(entity)) {
				auto &attack = registry.get<AttackComponent>(entity);
				if (attack.attackTimer.isRunning() && !attack.attackTimer.isTimeout()) {
					auto &input = registry.get<InputComponent>(entity);
					input.horizontalDirection = 0.0f;
					continue;
				}
			}
			const auto &[position, previousPosition] = registry.get<PositionComponent>(entity);
			const auto &[minBounds, maxBounds] = registry.get<PatrolComponent>(entity);
			auto &input = registry.get<InputComponent>(entity);
			const auto &[collisionBox] = registry.get<CollisionComponent>(entity);

			auto scale = glm::vec2(1.0f, 1.0f);
			if (registry.has<TransformComponent>(entity)) {
				scale = registry.get<TransformComponent>(entity).scale;
			}

			const float enemyRight = position.x + (collisionBox.x + collisionBox.w) * scale.x;
			const float enemyLeft = position.x + collisionBox.x * scale.x;
			if (input.horizontalDirection == 0.0f) {
				if (registry.has<ChaseComponent>(entity)) {
					if (const auto &chase = registry.get<ChaseComponent>(entity); chase.lastKnownPlayerX) {
						const float enemyCenterX = (enemyLeft + enemyRight) * 0.5f;
						input.horizontalDirection = chase.lastKnownPlayerX > enemyCenterX ? 1.0f : -1.0f;
					} else {
						input.horizontalDirection = 1.0f;
					}
				} else {
					input.horizontalDirection = 1.0f;
				}
			}

			if (input.horizontalDirection > 0 && enemyRight >= maxBounds.x) {
				input.horizontalDirection = -1.0f;
			} else if (input.horizontalDirection < 0 && enemyLeft <= minBounds.x) {
				input.horizontalDirection = 1.0f;
			}
		}
	}
};
} // namespace GameEngine::ECS::Systems