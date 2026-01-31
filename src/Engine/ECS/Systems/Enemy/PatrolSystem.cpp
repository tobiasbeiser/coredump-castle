#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/AiStateComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/States/StateMachineComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/CollisionSystem.hpp"
#include "Engine/ECS/View.hpp"

namespace GameEngine::ECS::Systems::PatrolSystem {

// Reusable logic for patrol movement
void updateEntity(Registry &registry, Entity entity)
{
	// Skip AiStateComponent check for entities with state machines - they handle their own state
	if (!registry.has<StateMachineComponent>(entity)) {
		// Only process entities in Patrol state if they have one
		if (const auto stateComp = registry.tryGet<AiStateComponent>(entity)) {
			if (stateComp->get().state != AiState::Patrol) {
				return;
			}
		}
	}

	const auto patrolComp = registry.tryGet<PatrolComponent>(entity);
	if (!patrolComp)
		return;
	const auto &[patrolSpeed] = patrolComp->get();

	auto movementComp = registry.tryGet<MovementComponent>(entity);
	if (!movementComp)
		return;
	auto &[accelerationRate, maxSpeedX] = movementComp->get();
	maxSpeedX = patrolSpeed;

	auto inputComp = registry.tryGet<InputComponent>(entity);
	if (!inputComp)
		return;
	auto &input = inputComp->get();

	const auto posComp = registry.tryGet<PositionComponent>(entity);
	if (!posComp)
		return;
	const auto &[position, previousPosition] = posComp->get();

	const auto colComp = registry.tryGet<CollisionComponent>(entity);
	if (!colComp)
		return;
	const auto &[collisionBox] = colComp->get();

	const auto transform = registry.tryGet<TransformComponent>(entity);
	const std::optional<const glm::vec2> scale = transform ? std::optional(transform->get().scale) : std::nullopt;

	// Ensure moving if stopped
	if (input.horizontalDirection == 0.0f) {
		input.horizontalDirection = 1.0f;
	}

	const bool isEnvironmentBlocked =
	    !CollisionSystem::canContinueMovingHorizontally(position, collisionBox, scale, input.horizontalDirection);

	if (isEnvironmentBlocked) {
		input.horizontalDirection *= -1.0f;
	}
}

void update(Registry &registry)
{
	for (auto view = registry.view<PositionComponent, PatrolComponent, InputComponent, CollisionComponent>();
	     const auto entity : view) {

		// Skip entities with state machines - they handle patrol in their own states
		if (registry.has<StateMachineComponent>(entity)) {
			continue;
		}

		updateEntity(registry, entity);
	}
};

} // namespace GameEngine::ECS::Systems::PatrolSystem