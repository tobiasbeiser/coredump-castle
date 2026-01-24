#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/RangedAttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/Combat/AttackSystem.hpp"
#include "Engine/ECS/View.hpp"

namespace GameEngine::ECS::Systems::PatrolSystem {

void update(Registry &registry)
{
	for (auto view = registry.view<PositionComponent, PatrolComponent, InputComponent, CollisionComponent>();
	     const auto entity : view) {

		if (const auto attack = registry.tryGet<AttackComponent>(entity)) {
			if (attack->get().attackTimer.isRunning() && !attack->get().attackTimer.isTimeout()) {
				auto &input = view.get<InputComponent>(entity);
				input.horizontalDirection = 0.0f;
				continue;
			}
		}
		const auto &[position, previousPosition] = view.get<PositionComponent>(entity);
		const auto &[minBounds, maxBounds] = view.get<PatrolComponent>(entity);
		auto &input = view.get<InputComponent>(entity);
		const auto &[collisionBox] = view.get<CollisionComponent>(entity);

		const auto transform = registry.tryGet<TransformComponent>(entity);
		const glm::vec2 scale = transform ? transform->get().scale : glm::vec2{1.0f, 1.0f};

		const float enemyRight = position.x + (collisionBox.x + collisionBox.w) * scale.x;
		const float enemyLeft = position.x + collisionBox.x * scale.x;

		bool isChasing = false;
		if (const auto chase = registry.tryGet<ChaseComponent>(entity)) {
			if (chase->get().lastKnownPlayerX.has_value()) {
				isChasing = true;
			}
		}

		if (input.horizontalDirection == 0.0f && !isChasing) {
			input.horizontalDirection = 1.0f;
		}

		if (input.horizontalDirection > 0 && enemyRight >= maxBounds.x) {
			if (isChasing) {
				input.horizontalDirection = 0.0f;
			} else {
				input.horizontalDirection = -1.0f;
			}
		} else if (input.horizontalDirection < 0 && enemyLeft <= minBounds.x) {
			if (isChasing) {
				input.horizontalDirection = 0.0f;
			} else {
				input.horizontalDirection = 1.0f;
			}
		}
	}
};

} // namespace GameEngine::ECS::Systems::PatrolSystem