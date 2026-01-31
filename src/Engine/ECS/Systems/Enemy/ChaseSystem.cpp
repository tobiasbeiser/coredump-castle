#include "ChaseSystem.hpp"
#include "Engine/Debug/ScopedProfiler.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/AiStateComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChargeComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/States/StateMachineComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/CollisionSystem.hpp"
#include "Engine/ECS/Systems/Combat/AttackSystem.hpp"
#include "Engine/ECS/View.hpp"

#include "Engine/Utils/DetectionRangeUtils.hpp"
#include "Engine/Utils/HitBoxUtils.hpp"
#include "Engine/Utils/Random.hpp"

namespace {
struct PlayerContext {
	glm::vec2 center;
};

std::optional<PlayerContext> getPlayerContext(GameEngine::ECS::Registry &registry)
{
	auto view = registry.view<PlayerTag, PositionComponent, HurtBoxComponent, TransformComponent>();
	for (const auto entity : view) {
		const auto &pos = view.get<PositionComponent>(entity).position;
		const auto &hurtBox = view.get<HurtBoxComponent>(entity).hurtBox;
		const auto &transform = view.get<TransformComponent>(entity);

		return PlayerContext{GameEngine::Utils::DetectionRangeUtils::calculateCenter(pos, hurtBox, transform.scale)};
	}
	return std::nullopt;
}
} // namespace

void GameEngine::ECS::Systems::ChaseSystem::update(Registry &registry, const float deltaTime)
{
	PROFILE_SCOPE("ChaseSystem");

	const auto playerCtxOpt = getPlayerContext(registry);

	if (!playerCtxOpt.has_value()) {
		return;
	}
	const PlayerContext &playerCtx = playerCtxOpt.value();

	auto enemyView = registry.view<InputComponent, ChaseComponent, PositionComponent, HitBoxComponent,
	                               CollisionComponent, MovementComponent, AiStateComponent>();

	for (const auto entity : enemyView) {
		// Skip entities with state machines - they handle their own behavior
		if (registry.has<StateMachineComponent>(entity)) {
			continue;
		}

		if (auto &[state] = enemyView.get<AiStateComponent>(entity); state != AiState::Chase) {
			continue;
		}

		auto &pos = enemyView.get<PositionComponent>(entity);

		auto &chase = enemyView.get<ChaseComponent>(entity);
		auto &input = enemyView.get<InputComponent>(entity);
		auto &movement = enemyView.get<MovementComponent>(entity);
		auto &[hitBox] = enemyView.get<HitBoxComponent>(entity);
		auto &[collisionBox] = enemyView.get<CollisionComponent>(entity);

		const auto &transform = registry.tryGet<TransformComponent>(entity);

		const auto enemyScale = transform ? &transform->get().scale : nullptr;

		const auto entityHitBox = Utils::HitBoxUtils::getHitBox(pos.position, hitBox, enemyScale);

		chase.reactionDelay.step(std::chrono::duration<float>(deltaTime), false);
		if (!chase.reactionDelay.isTimeout()) {
			input.horizontalDirection = 0.0f;
			continue;
		}

		const float direction = playerCtx.center.x > entityHitBox.x + entityHitBox.w / 2 ? 1.0f : -1.0f;

		const bool isEnvironmentBlocked =
		    !CollisionSystem::canContinueMovingHorizontally(pos.position, collisionBox, *enemyScale, direction);

		if (!isEnvironmentBlocked) {
			input.horizontalDirection = direction;
			movement.maxSpeedX = chase.chaseSpeed;
		} else {
			input.horizontalDirection = 0;
		}
	}
}