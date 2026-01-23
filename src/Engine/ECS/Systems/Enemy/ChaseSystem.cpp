#include "Engine/Debug/ScopedProfiler.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/ProjectileComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/RangedAttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/CollisionSystem.hpp"
#include "Engine/ECS/Systems/Combat/AttackSystem.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/Utils/DetectionRangeUtils.hpp"
#include "Engine/Utils/SpatialHash.hpp"

namespace GameEngine::ECS::Systems::ChaseSystem {

namespace {

struct PlayerContext {
	glm::vec2 center = glm::vec2(0.0f);
	SDL_FRect hurtBox = {0, 0, 0, 0};
	glm::vec2 scale = glm::vec2(1.0f, 1.0f);
	bool isGrounded = false;
	const StateComponent *state = nullptr;
};

struct EnemyContext {
	Entity entity;
	InputComponent *input = nullptr;
	ChaseComponent *chase = nullptr;
	MovementComponent *movement = nullptr;
	RenderComponent *render = nullptr;
	AttackComponent *attack = nullptr;
	const PatrolComponent *patrol = nullptr;

	SDL_FRect hurtBox;
	glm::vec2 center;
	glm::vec2 scale;
	glm::vec2 position;
};
void resetChaseState(const EnemyContext &enemy)
{
	enemy.input->attackPressed = false;
	enemy.movement->maxSpeedX = enemy.chase->originalPatrolSpeed;
	enemy.chase->reactionDelay.reset();
	enemy.chase->lastKnownPlayerX = std::nullopt;
}

std::optional<PlayerContext> getPlayerContext(Registry &registry)
{
	auto playerView =
	    registry.view<PlayerTag, PositionComponent, HurtBoxComponent, TransformComponent, GroundedComponent>();
	for (auto view =
	         registry.view<PlayerTag, PositionComponent, HurtBoxComponent, TransformComponent, GroundedComponent>();
	     const auto entity : view) {

		const auto &pos = registry.get<PositionComponent>(entity).position;
		const auto &hurtBox = registry.get<HurtBoxComponent>(entity).hurtBox;
		const auto &transform = registry.get<TransformComponent>(entity);
		const auto &ground = registry.get<GroundedComponent>(entity);
		const auto &state = registry.get<StateComponent>(entity);

		return PlayerContext{Utils::DetectionRangeUtils::calculateCenter(pos, hurtBox, transform.scale), hurtBox,
		                     transform.scale, ground.grounded, &state};
	}
	return std::nullopt;
}

bool checkLineOfSight(const EnemyContext &enemy, const PlayerContext &player, const Utils::SpatialHash &grid,
                      RangedAttackComponent &ranged, const glm::vec2 &projectileSize, const glm::vec2 &hitboxOffset)
{
	const bool facingLeft = (player.center.x - enemy.center.x) < 0.0f;

	glm::vec2 spawnPosition = enemy.position;
	float xOffset = (enemy.hurtBox.x * enemy.scale.x);
	xOffset += facingLeft ? -ranged.projectileOffset.x : ranged.projectileOffset.x;

	spawnPosition.x += xOffset;
	spawnPosition.y += ranged.projectileOffset.y;

	const glm::vec2 rayStart = spawnPosition + hitboxOffset + (projectileSize * 0.5f);

	const glm::vec2 playerTop = player.center - glm::vec2(0.0f, player.hurtBox.h * player.scale.y * 0.25f);

	for (const std::vector targetPoints = {player.center, playerTop}; const auto &targetPoint : targetPoints) {

		if (const bool obstructed = CollisionSystem::isPathObstructed(rayStart, targetPoint, grid, projectileSize);
		    !obstructed) {
			ranged.targetSnapshot = targetPoint;
			return true;
		}
	}
	return false;
}

void handleRangeAttack(const EnemyContext &enemy, const PlayerContext &player, const Utils::SpatialHash &grid,
                       Registry &registry)
{

	auto &ranged = registry.get<RangedAttackComponent>(enemy.entity);

	if (player.state->state == State::Dead) {
		resetChaseState(enemy);
		enemy.attack->attackTimer.reset();
		ranged.targetSnapshot = glm::vec2(0.0f);
		ranged.projectileSpawned = false;
		return;
	}

	const float directionToPlayer = player.center.x > enemy.center.x ? 1.0f : -1.0f;
	const bool withinRange =
	    Utils::DetectionRangeUtils::isWithinDetectionRange(enemy.center, player.center, *enemy.chase);

	if (!withinRange && !AttackSystem::isAttacking(*enemy.attack)) {
		resetChaseState(enemy);
		return;
	}

	if (withinRange) {
		enemy.chase->lastKnownPlayerX = player.center.x;
	}

	float progress = 0.0f;
	if (AttackSystem::isAttacking(*enemy.attack) && enemy.attack->attackTimer.getLength().count() > 0.0f) {
		progress = enemy.attack->attackTimer.getTime().count() / enemy.attack->attackTimer.getLength().count();
	}

	if (progress <= enemy.attack->windUpLimit) {
		glm::vec2 projectileSize = {0.f, 0.f};
		glm::vec2 hitboxOffset = {0.0f, 0.0f};
		if (registry.has<ProjectileComponent>(enemy.entity)) {
			const auto &pc = registry.get<ProjectileComponent>(enemy.entity);
			projectileSize = pc.projectileSize;
			hitboxOffset = pc.hitboxOffset;
		}

		if (!checkLineOfSight(enemy, player, grid, ranged, projectileSize, hitboxOffset)) {
			resetChaseState(enemy);
			enemy.attack->attackTimer.reset();
			return;
		}
	}

	// player is close enough
	enemy.chase->lastKnownPlayerX = player.center.x;
	enemy.input->horizontalDirection = 0.0f;
	enemy.render->flipX = directionToPlayer < 0;
	enemy.input->attackPressed = enemy.chase->reactionDelay.isTimeout();
}

void handleMeleeAttack(const EnemyContext &enemy, const PlayerContext &player)
{
	if (player.center.x < enemy.patrol->minBounds.x || player.center.x > enemy.patrol->maxBounds.x) {
		resetChaseState(enemy);
		return;
	}

	const float verticalThreshold = Utils::DetectionRangeUtils::getVerticalThreshold(*enemy.chase, player.isGrounded);

	if (std::abs(player.center.y - enemy.center.y) > verticalThreshold) {
		resetChaseState(enemy);
		return;
	}

	enemy.chase->lastKnownPlayerX = player.center.x;

	if (AttackSystem::isAttacking(*enemy.attack)) {
		enemy.input->attackPressed = false;
		enemy.input->horizontalDirection = 0.0f;
		return;
	}

	const float xDistance = std::abs(player.center.x - enemy.center.x);
	const float directionToPlayer = player.center.x > enemy.center.x ? 1.0f : -1.0f;

	if (const float stopDistance =
	        (player.hurtBox.w * player.scale.x * 0.5f) + (enemy.hurtBox.w * enemy.scale.x * 0.5f);
	    xDistance <= stopDistance + 5.0f) {
		enemy.input->horizontalDirection = 0.0f;
		enemy.render->flipX = directionToPlayer < 0;
		enemy.input->attackPressed = enemy.chase->reactionDelay.isTimeout();
	} else {
		enemy.input->horizontalDirection = directionToPlayer;
		enemy.render->flipX = directionToPlayer < 0;
		enemy.movement->maxSpeedX = enemy.chase->chaseSpeed;
		enemy.input->attackPressed = false;
		enemy.chase->reactionDelay.reset();
	}
}

} // namespace

void update(Registry &registry, const float deltaTime)
{
	PROFILE_SCOPE("ChaseSystem");

	const auto playerCtxOpt = getPlayerContext(registry);

	if (!playerCtxOpt.has_value()) {
		return;
	}
	const PlayerContext &playerCtx = playerCtxOpt.value();

	const auto staticColliderGrid = CollisionSystem::buildStaticColliderGrid(registry);

	auto enemyView = registry.view<EnemyTag, InputComponent, PatrolComponent, ChaseComponent, PositionComponent,
	                               HitBoxComponent, HurtBoxComponent, CollisionComponent, MovementComponent,
	                               GroundedComponent, AttackComponent, RenderComponent>();

	for (const auto entity : enemyView) {
		const auto &pos = registry.get<PositionComponent>(entity);
		const auto &enemyHurtBox = registry.get<HurtBoxComponent>(entity).hurtBox;
		auto &chase = registry.get<ChaseComponent>(entity);

		glm::vec2 enemyScale = {1.0f, 1.0f};
		if (registry.has<TransformComponent>(entity)) {
			enemyScale = registry.get<TransformComponent>(entity).scale;
		}

		EnemyContext enemyCtx{entity,
		                      &registry.get<InputComponent>(entity),
		                      &registry.get<ChaseComponent>(entity),
		                      &registry.get<MovementComponent>(entity),
		                      &registry.get<RenderComponent>(entity),
		                      &registry.get<AttackComponent>(entity),
		                      &registry.get<PatrolComponent>(entity),
		                      enemyHurtBox,
		                      Utils::DetectionRangeUtils::calculateCenter(pos.position, enemyHurtBox, enemyScale),
		                      enemyScale,
		                      pos.position};

		chase.reactionDelay.step(std::chrono::duration<float>(deltaTime), false);

		if (registry.has<RangedAttackComponent>(entity)) {
			handleRangeAttack(enemyCtx, playerCtx, staticColliderGrid, registry);
		} else {
			handleMeleeAttack(enemyCtx, playerCtx);
		}
	}
}

} // namespace GameEngine::ECS::Systems::ChaseSystem
