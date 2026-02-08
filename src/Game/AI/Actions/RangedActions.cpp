#include "RangedActions.hpp"

#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/ProjectileComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/RangedAttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
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
	if (const auto input = registry.tryGet<InputComponent>(entity))
		input->get().attackPressed = false;
	if (const auto attack = registry.tryGet<AttackComponent>(entity))
		attack->get().attackTimer.reset();
	if (const auto ranged = registry.tryGet<RangedAttackComponent>(entity)) {
		ranged->get().targetSnapshot = glm::vec2(0.0f);
		ranged->get().projectileSpawned = false;
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

} // namespace Transitions

} // namespace GameEngine::ECS
