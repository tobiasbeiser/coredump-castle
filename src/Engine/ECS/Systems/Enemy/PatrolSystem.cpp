#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/CollisionSystem.hpp"
#include "Engine/ECS/View.hpp"

namespace GameEngine::ECS::Systems::PatrolSystem {

void updateEntity(Registry &registry, Entity entity)
{
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
	// All patrol enemies now use state machines
	// This function is kept for backwards compatibility but does nothing
	// Patrol logic is handled via PatrolAction in state machine updates
}

} // namespace GameEngine::ECS::Systems::PatrolSystem