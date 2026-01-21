#include "Engine/Debug/ScopedProfiler.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/Combat/AttackSystem.hpp"
#include "Engine/ECS/View.hpp"

namespace GameEngine::ECS::Systems::ChaseSystem {

namespace {

void resetChaseState(InputComponent &input, MovementComponent &movement, ChaseComponent &chase)
{
	input.attackPressed = false;
	movement.maxSpeedX = chase.originalPatrolSpeed;
	chase.reactionDelay.reset();
}

} // namespace

void update(Registry &registry, float deltaTime)
{
	PROFILE_SCOPE("ChaseSystem");

	auto playerView =
	    registry.view<PlayerTag, PositionComponent, HurtBoxComponent, TransformComponent, GroundedComponent>();

	glm::vec2 playerCenterPos = {0, 0};
	SDL_FRect playerHurtBox = {0, 0, 0, 0};
	glm::vec2 playerScale = {1.0f, 1.0f};
	bool playerGrounded = false;

	for (const auto entity : playerView) {
		const auto &pos = registry.get<PositionComponent>(entity).position;
		playerHurtBox = registry.get<HurtBoxComponent>(entity).hurtBox;
		playerScale = registry.get<TransformComponent>(entity).scale;
		playerGrounded = registry.get<GroundedComponent>(entity).grounded;

		playerCenterPos =
		    glm::vec2(pos.x + (playerHurtBox.x * playerScale.x) + (playerHurtBox.w * playerScale.x) * 0.5f,
		              pos.y + (playerHurtBox.y * playerScale.y) + (playerHurtBox.h * playerScale.y) * 0.5f);
		break;
	}

	auto enemyView = registry.view<EnemyTag, InputComponent, PatrolComponent, ChaseComponent, PositionComponent,
	                               HitBoxComponent, HurtBoxComponent, CollisionComponent, MovementComponent,
	                               GroundedComponent, AttackComponent, RenderComponent>();

	for (const auto entity : enemyView) {
		const auto &pos = registry.get<PositionComponent>(entity);
		const auto &hurtBox = registry.get<HurtBoxComponent>(entity).hurtBox;
		const auto &patrol = registry.get<PatrolComponent>(entity);
		const auto &attack = registry.get<AttackComponent>(entity);

		auto &input = registry.get<InputComponent>(entity);
		auto &chase = registry.get<ChaseComponent>(entity);
		auto &movement = registry.get<MovementComponent>(entity);
		auto &render = registry.get<RenderComponent>(entity);

		glm::vec2 scale = {1.0f, 1.0f};
		if (registry.has<TransformComponent>(entity)) {
			scale = registry.get<TransformComponent>(entity).scale;
		}

		chase.reactionDelay.step(std::chrono::duration<float>(deltaTime), false);

		if (playerCenterPos.x < patrol.minBounds.x || playerCenterPos.x > patrol.maxBounds.x) {
			resetChaseState(input, movement, chase);
			continue;
		}

		const glm::vec2 enemyCenter = {pos.position.x + (hurtBox.x * scale.x) + (hurtBox.w * scale.x) * 0.5f,
		                               pos.position.y + (hurtBox.y * scale.y) + (hurtBox.h * scale.y) * 0.5f};

		const float verticalThreshold =
		    playerGrounded ? chase.verticalDetectionHeight : chase.verticalDetectionHeight * 2.0f;

		if (std::abs(playerCenterPos.y - enemyCenter.y) > verticalThreshold) {
			resetChaseState(input, movement, chase);
			continue;
		}

		if (AttackSystem::isAttacking(attack)) {
			input.attackPressed = false;
			input.horizontalDirection = 0.0f;
			continue;
		}

		const float xDistance = std::abs(playerCenterPos.x - enemyCenter.x);
		const float directionToPlayer = playerCenterPos.x > enemyCenter.x ? 1.0f : -1.0f;

		if (const float stopDistance = (playerHurtBox.w * playerScale.x * 0.5f) + (hurtBox.w * scale.x * 0.5f);
		    xDistance <= stopDistance) {
			input.horizontalDirection = 0.0f;
			render.flipX = directionToPlayer < 0;
			input.attackPressed = chase.reactionDelay.isTimeout();
		} else {
			input.horizontalDirection = directionToPlayer;
			render.flipX = directionToPlayer < 0;
			movement.maxSpeedX = chase.chaseSpeed;
			input.attackPressed = false;
			chase.reactionDelay.reset();
		}
	}
}

} // namespace GameEngine::ECS::Systems::ChaseSystem
