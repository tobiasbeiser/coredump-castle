#include "RangedActions.hpp"

#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/ProjectileComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/RangedAttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Systems/Combat/AttackSystem.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/Utils/DetectionRangeUtils.hpp"
#include "Engine/Utils/Random.hpp"
#include "Game/Factories/ProjectileFactory.hpp"
namespace GameEngine::ECS {

namespace {
struct ProjectileInfo {
	glm::vec2 size{0.0f, 0.0f};
	glm::vec2 offset{0.0f, 0.0f};
};

ProjectileInfo getProjectileInfo(const ProjectileComponent *projComp)
{
	if (!projComp || projComp->projectileTilesetPath.empty())
		return {};

	const auto &data = ProjectileFactory::getCachedProjectileData(projComp->projectileTilesetPath);
	return {{data.hitbox.w, data.hitbox.h}, {data.hitbox.x, data.hitbox.y}};
}

bool hasLosToAttacker(const EntityInfo &enemy, const EntityInfo &player, const RangedAttackComponent *ranged,
                      const ProjectileComponent *proj)
{
	auto [size, offset] = getProjectileInfo(proj);

	const bool facingLeft = (player.center.x - enemy.position.x) < 0.0f;

	glm::vec2 spawnPos = enemy.position;
	const float relativeHurtboxX = enemy.hitBoxRect.x - enemy.position.x;
	float xOffset = relativeHurtboxX;

	if (ranged) {
		xOffset += facingLeft ? -ranged->projectileOffset.x : ranged->projectileOffset.x;
		spawnPos.y += ranged->projectileOffset.y;
	}
	spawnPos.x += xOffset;

	const glm::vec2 rayStart = spawnPos + offset + (size * 0.5f);

	const auto targetOpt = Utils::DetectionRangeUtils::getTargetWithLineOfSight(rayStart, player.center,
	                                                                            player.hitBoxRect, player.scale, size);

	return targetOpt.has_value();
}

} // namespace

namespace Actions {

void AimAtPlayerWithLOSAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	auto enemy = getEntityInfo(registry, entity);
	if (!enemy)
		return;

	auto rangedOpt = registry.tryGet<RangedAttackComponent>(entity);
	if (!rangedOpt)
		return;

	auto projOpt = registry.tryGet<ProjectileComponent>(entity);
	auto renderOpt = registry.tryGet<RenderComponent>(entity);
	if (!renderOpt)
		return;

	auto player = getPlayerInfo(registry);
	if (!player) {
		rangedOpt->get().targetSnapshot = glm::vec2(0.0f);
		return;
	}

	auto [size, offset] = getProjectileInfo(projOpt ? &projOpt->get() : nullptr);
	bool facingLeft = renderOpt->get().isFacingLeft();

	glm::vec2 spawnPos = enemy->position;
	float relativeHurtboxX = enemy->hitBoxRect.x - enemy->position.x;
	float xOffset = relativeHurtboxX;

	xOffset += facingLeft ? -rangedOpt->get().projectileOffset.x : rangedOpt->get().projectileOffset.x;
	spawnPos.y += rangedOpt->get().projectileOffset.y;
	spawnPos.x += xOffset;

	const glm::vec2 rayStart = spawnPos + offset + (size * 0.5f);

	auto targetOpt = Utils::DetectionRangeUtils::getTargetWithLineOfSight(rayStart, player->center, player->hitBoxRect,
	                                                                      player->scale, size);

	if (targetOpt) {
		rangedOpt->get().targetSnapshot = *targetOpt;
	} else {
		rangedOpt->get().targetSnapshot = glm::vec2(0.0f);
	}
}

void TriggerRangedAttackAction::execute(Registry &registry, const Entity entity, float deltaTime)
{
	const auto inputOpt = registry.tryGet<InputComponent>(entity);
	const auto attackOpt = registry.tryGet<AttackComponent>(entity);
	const auto rangedOpt = registry.tryGet<RangedAttackComponent>(entity);

	if (!inputOpt || !attackOpt || !rangedOpt)
		return;

	auto &input = inputOpt->get();
	auto &attack = attackOpt->get();
	auto &ranged = rangedOpt->get();

	for (auto playerView = registry.view<PlayerTag, StateComponent>(); const auto playerEntity : playerView) {
		if (playerView.get<StateComponent>(playerEntity).state == State::Dead) {
			input.attackPressed = false;
			attack.attackTimer.reset();
			ranged.targetSnapshot = glm::vec2(0.0f);
			ranged.projectileSpawned = false;
			return;
		}
		break;
	}

	if (ranged.targetSnapshot == glm::vec2(0.0f)) {
		input.attackPressed = false;
		attack.attackTimer.reset();
		return;
	}

	if (!Systems::AttackSystem::isAttacking(attack)) {
		input.attackPressed = true;
		attack.activeWindUpLimit = Utils::Random::getFloat(attack.windUpLimit * 0.5f, attack.windUpLimit);
	}
}

void CancelRangedAttackAction::execute(Registry &registry, const Entity entity, float deltaTime)
{
	if (const auto input = registry.tryGet<InputComponent>(entity)) {
		input->get().attackPressed = false;
		input->get().jumpPressed = false;
	}
	if (const auto attack = registry.tryGet<AttackComponent>(entity))
		attack->get().attackTimer.reset();
	if (const auto ranged = registry.tryGet<RangedAttackComponent>(entity)) {
		ranged->get().targetSnapshot = glm::vec2(0.0f);
		ranged->get().projectileSpawned = false;
	}
}

void Actions::RetreatFromPlayerAction::execute(Registry &registry, const Entity entity, float deltaTime)
{
	const auto enemy = getEntityInfo(registry, entity);
	const auto player = getPlayerInfo(registry);

	if (!enemy || !player)
		return;

	auto inputOpt = registry.tryGet<InputComponent>(entity);
	auto renderOpt = registry.tryGet<RenderComponent>(entity);
	auto movementOpt = registry.tryGet<MovementComponent>(entity);
	auto stateOpt = registry.tryGet<StateComponent>(entity);

	if (!inputOpt || !renderOpt)
		return;

	// Move away from player
	const bool playerOnLeft = player->center.x < enemy->center.x;

	// Face the player while retreating (to shoot at them)
	renderOpt->get().setFacingLeft(playerOnLeft);

	// Move away from player
	if (playerOnLeft) {
		inputOpt->get().horizontalDirection = 1.0f; // Move right
	} else {
		inputOpt->get().horizontalDirection = -1.0f; // Move left
	}

	// Set retreat speed
	if (movementOpt) {
		movementOpt->get().maxSpeedX = 30;
	}

	// Set state to Rolling so AnimationSystem uses Roll animation
	if (stateOpt) {
		stateOpt->get().state = State::Rolling;
	}
}

void Actions::RecordCornerAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	// Debug logging to trace crash
	// std::cout << "RecordCornerAction executing for entity " << entity << std::endl;
	auto patrolOpt = registry.tryGet<PatrolComponent>(entity);
	const auto renderOpt = registry.tryGet<RenderComponent>(entity);
	const auto posOpt = registry.tryGet<PositionComponent>(entity);

	if (!patrolOpt || !renderOpt || !posOpt) {
		// std::cout << "Missing components in RecordCornerAction" << std::endl;
		return;
	}

	// If we are cornered, we are at a wall. Record this position.
	if (renderOpt->get().isFacingLeft()) {
		// Facing left and stuck -> Left Wall (MinX)
		// We use current position as a safe estimate for the wall
		if (!patrolOpt->get().minX.has_value() || posOpt->get().position.x < patrolOpt->get().minX.value()) {
			// std::cout << "Recording MinX: " << posOpt->get().position.x << std::endl;
			patrolOpt->get().minX = posOpt->get().position.x;
		}
	} else {
		// Facing right and stuck -> Right Wall (MaxX)
		if (!patrolOpt->get().maxX.has_value() || posOpt->get().position.x > patrolOpt->get().maxX.value()) {
			// std::cout << "Recording MaxX: " << posOpt->get().position.x << std::endl;
			patrolOpt->get().maxX = posOpt->get().position.x;
		}
	}
}

void Actions::EvadeJumpAction::execute(Registry &registry, const Entity entity, float deltaTime)
{
	const auto enemy = getEntityInfo(registry, entity);
	const auto player = getPlayerInfo(registry);

	if (!enemy || !player)
		return;

	auto inputOpt = registry.tryGet<InputComponent>(entity);
	auto renderOpt = registry.tryGet<RenderComponent>(entity);
	auto movementOpt = registry.tryGet<MovementComponent>(entity);
	auto groundedOpt = registry.tryGet<GroundedComponent>(entity);
	auto patrolOpt = registry.tryGet<PatrolComponent>(entity);

	if (!inputOpt || !renderOpt)
		return;

	// Trigger jump on first frame (when grounded)
	if (groundedOpt && groundedOpt->get().grounded) {
		inputOpt->get().jumpPressed = true;
	} else {
		inputOpt->get().jumpPressed = false;
	}

	// Move away from player during jump
	const bool playerOnLeft = player->center.x < enemy->center.x;

	// Face the player while jumping (to look menacing)
	renderOpt->get().setFacingLeft(playerOnLeft);

	// Move away from player
	// Determine best jump target (Center, Left Wall, Right Wall)
	// We want to jump to the location that maximizes distance from player
	float bestTargetX = enemy->center.x;
	float maxDist = -1.0f;

	float dir = 0.0f;

	if (patrolOpt) {
		std::vector<float> candidates;
		candidates.push_back(patrolOpt->get().patrolCenter.x);
		if (patrolOpt->get().minX.has_value())
			candidates.push_back(patrolOpt->get().minX.value() + 50.0f); // Offset from wall
		if (patrolOpt->get().maxX.has_value())
			candidates.push_back(patrolOpt->get().maxX.value() - 50.0f); // Offset from wall

		for (float targetX : candidates) {
			float dist = std::abs(targetX - player->center.x);
			// Also ensure we don't jump INTO the player (cross them) if possible, or at least penalize close targets
			if (dist > maxDist) {
				maxDist = dist;
				bestTargetX = targetX;
			}
		}

		// Determine direction to best target
		dir = (bestTargetX > enemy->center.x) ? 1.0f : -1.0f;
	} else {
		// Fallback: away from player
		dir = playerOnLeft ? 1.0f : -1.0f;
	}

	inputOpt->get().horizontalDirection = dir;

	// Set movement speed AND force velocity if in air preventing standard movement
	if (movementOpt) {
		const float effectiveJumpSpeed = std::max(jumpSpeed, 400.0f);
		movementOpt->get().maxSpeedX = effectiveJumpSpeed;
		if (auto velOpt = registry.tryGet<VelocityComponent>(entity)) {
			// Directly apply horizontal velocity to ensure it moves even if air control is limited
			velOpt->get().velocity.x = dir * effectiveJumpSpeed;
		}
	}
}
} // namespace Actions


// Transitions

namespace Transitions {

bool LineOfSightTransition::canTransition(Registry &registry, const Entity entity, float deltaTime) const
{
	const auto enemy = getEntityInfo(registry, entity);
	const auto player = getPlayerInfo(registry);
	if (!enemy || !player)
		return false;

	const auto ranged = registry.tryGet<RangedAttackComponent>(entity);
	const auto proj = registry.tryGet<ProjectileComponent>(entity);

	return hasLosToAttacker(*enemy, *player, ranged ? &ranged->get() : nullptr, proj ? &proj->get() : nullptr);
}

bool LosLostTransition::canTransition(Registry &registry, const Entity entity, const float deltaTime) const
{
	const auto enemy = getEntityInfo(registry, entity);
	if (!enemy)
		return false;

	auto player = getPlayerInfo(registry);
	if (!player)
		return true;

	const auto rangedOpt = registry.tryGet<RangedAttackComponent>(entity);
	const auto projOpt = registry.tryGet<ProjectileComponent>(entity);

	if (hasLosToAttacker(*enemy, *player, rangedOpt ? &rangedOpt->get() : nullptr,
	                     projOpt ? &projOpt->get() : nullptr)) {
		losLostDuration = 0.0f;
		return false;
	}

	losLostDuration += deltaTime;
	return losLostDuration >= gracePeriod;
}

bool PlayerTooCloseTransition::canTransition(Registry &registry, const Entity entity,
														  float deltaTime) const
{
	const auto enemy = getEntityInfo(registry, entity);
	const auto player = getPlayerInfo(registry);

	if (!enemy || !player)
		return false;

	const float distance = glm::distance(enemy->center, player->center);
	return distance < closeRange;
}

bool IsGroundedTransition::canTransition(Registry &registry, const Entity entity, float deltaTime) const
{
	auto groundedOpt = registry.tryGet<GroundedComponent>(entity);
	if (!groundedOpt)
		return true;

	return groundedOpt->get().grounded;
}

bool LandedTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	const auto sm = registry.tryGet<StateMachineComponent>(entity);
	const auto grounded = registry.tryGet<GroundedComponent>(entity);

	if (!sm || !grounded)
		return false;
	if (sm->get().timeInState < minAirTime)
		return false;

	return grounded->get().grounded;
}

bool StuckTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	const auto sm = registry.tryGet<StateMachineComponent>(entity);
	const auto vel = registry.tryGet<VelocityComponent>(entity);

	if (!sm || !vel)
		return false;

	// Give it some time to accelerate
	if (sm->get().timeInState < 0.2f)
		return false;

	// If horizontal velocity is very low, we are likely stuck
	return std::abs(vel->get().velocity.x) < threshold;
} // namespace Transitions


} // namespace Transitions






} // namespace GameEngine::ECS
