#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/AiStateComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/CollisionSystem.hpp"
#include "Engine/ECS/View.hpp"

namespace GameEngine::ECS::Systems::PatrolSystem {

void update(Registry &registry)
{
	auto view = registry.view<PositionComponent, PatrolComponent, InputComponent, CollisionComponent, AiStateComponent,
	                          MovementComponent>();

	for (const auto entity : view) {

		if (const auto &[state] = view.get<AiStateComponent>(entity); state != AiState::Patrol) {
			continue;
		}
		const auto &[patrolSpeed] = view.get<PatrolComponent>(entity);
		auto &[accelerationRate, maxSpeedX] = view.get<MovementComponent>(entity);
		maxSpeedX = patrolSpeed;

		auto &input = view.get<InputComponent>(entity);
		const auto &[position, previousPosition] = view.get<PositionComponent>(entity);
		const auto &[collisionBox] = view.get<CollisionComponent>(entity);
		const auto transform = registry.tryGet<TransformComponent>(entity);
		const std::optional<const glm::vec2> scale = transform ? std::optional(transform->get().scale) : std::nullopt;

		if (input.horizontalDirection == 0.0f) {
			input.horizontalDirection = 1.0f;
		}

		const bool isEnvironmentBlocked =
		    !CollisionSystem::canContinueMovingHorizontally(position, collisionBox, scale, input.horizontalDirection);

		if (isEnvironmentBlocked) {
			input.horizontalDirection *= -1.0f;
		}
	}
}

} // namespace GameEngine::ECS::Systems::PatrolSystem