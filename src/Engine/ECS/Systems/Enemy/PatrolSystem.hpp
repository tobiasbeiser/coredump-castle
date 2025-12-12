#pragma once
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"

namespace GameEngine::ECS::Systems {
struct PatrolSystem {
	static void update(Registry &registry)
	{

		for (auto view = registry.view<PositionComponent, PatrolComponent, InputComponent, CollisionComponent>();
		     const auto entity : view) {
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
				input.horizontalDirection = 1.0f;
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