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
			const auto &[position, previousPosition] = view.get<PositionComponent>(entity);
			const auto &[minBounds, maxBounds] = view.get<PatrolComponent>(entity);
			auto &input = view.get<InputComponent>(entity);
			const auto &[collisionBox] = view.get<CollisionComponent>(entity);

			const auto transform = registry.tryGet<TransformComponent>(entity);
			const glm::vec2 scale = transform ? transform->get().scale : glm::vec2{1.0f, 1.0f};

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