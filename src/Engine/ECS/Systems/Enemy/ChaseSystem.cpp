#include "ChaseSystem.hpp"

#include "Engine/Debug/ScopedProfiler.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChargeComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/States/StateMachineComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/CollisionSystem.hpp"
#include "Engine/ECS/Systems/Combat/AttackSystem.hpp"
#include "Engine/ECS/Systems/Enemy/ChaseSystem.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/Utils/DetectionRangeUtils.hpp"
#include "Engine/Utils/HitBoxUtils.hpp"
#include "Engine/Utils/Random.hpp"
#include "Game/Factories/ProjectileFactory.hpp"

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

void GameEngine::ECS::Systems::ChaseSystem::updateEntity(GameEngine::ECS::Registry &registry, Entity entity,
                                                         const float deltaTime)
{
	const auto playerCtxOpt = getPlayerContext(registry);
	if (!playerCtxOpt.has_value()) {
		return;
	}
	const PlayerContext &playerCtx = playerCtxOpt.value();

	auto pos = registry.tryGet<PositionComponent>(entity);
	auto chase = registry.tryGet<ChaseComponent>(entity);
	auto input = registry.tryGet<InputComponent>(entity);
	auto movement = registry.tryGet<MovementComponent>(entity);
	auto hitBox = registry.tryGet<HitBoxComponent>(entity);
	auto collision = registry.tryGet<CollisionComponent>(entity);

	if (!pos || !chase || !input || !movement || !hitBox || !collision) {
		return;
	}

	const auto transform = registry.tryGet<TransformComponent>(entity);
	const auto enemyScale = transform ? &transform->get().scale : nullptr;
	const glm::vec2 actualScale = enemyScale ? *enemyScale : glm::vec2{1.0f, 1.0f};

	const auto entityHitBox =
	    GameEngine::Utils::HitBoxUtils::getHitBox(pos->get().position, hitBox->get().bodyHitBox, enemyScale);

	chase->get().reactionDelay.step(std::chrono::duration<float>(deltaTime), false);
	if (!chase->get().reactionDelay.isTimeout()) {
		input->get().horizontalDirection = 0.0f;
		return;
	}

	const float direction = playerCtx.center.x > entityHitBox.x + entityHitBox.w / 2 ? 1.0f : -1.0f;

	const bool isEnvironmentBlocked = !GameEngine::ECS::Systems::CollisionSystem::canContinueMovingHorizontally(
	    pos->get().position, collision->get().collisionBox, actualScale, direction);

	if (!isEnvironmentBlocked) {
		input->get().horizontalDirection = direction;
		movement->get().maxSpeedX = chase->get().chaseSpeed;
	} else {
		input->get().horizontalDirection = 0;
	}
}

void GameEngine::ECS::Systems::ChaseSystem::update(Registry &registry, const float deltaTime)
{
	PROFILE_SCOPE("ChaseSystem");

	// All chase behavior is now handled by state machines
	// This function is kept for backwards compatibility but does nothing
	// Chase logic is handled via MoveTowardsPlayerAction in state machine updates
}