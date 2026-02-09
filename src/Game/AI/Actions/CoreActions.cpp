#include "CoreActions.hpp"

#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"

#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/AccelerationComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Render/AnimationComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/Enemy/PatrolSystem.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/Utils/HitBoxUtils.hpp"
#include "Engine/Utils/Random.hpp"

#include <cmath>

namespace GameEngine::ECS {

std::optional<EntityInfo> getEntityInfo(Registry &registry, Entity entity)
{
	const auto posOpt = registry.tryGet<PositionComponent>(entity);
	const auto hurtBoxOpt = registry.tryGet<HurtBoxComponent>(entity);

	if (!posOpt || !hurtBoxOpt)
		return std::nullopt;

	const auto transformOpt = registry.tryGet<TransformComponent>(entity);
	const glm::vec2 scale = transformOpt ? transformOpt->get().scale : glm::vec2{1.0f, 1.0f};
	const SDL_FRect hitBoxRect =
	    Utils::HitBoxUtils::getHitBox(posOpt->get().position, hurtBoxOpt->get().hurtBox, &scale);

	return EntityInfo{posOpt->get().position,
	                  hitBoxRect,
	                  {hitBoxRect.x + hitBoxRect.w * 0.5f, hitBoxRect.y + hitBoxRect.h * 0.5f},
	                  scale};
}

std::optional<EntityInfo> getPlayerInfo(Registry &registry)
{
	auto playerView = registry.view<PlayerTag, PositionComponent, HurtBoxComponent>();
	if (playerView.size() == 0) {
		return std::nullopt;
	}
	
	for (const auto entity : playerView) {
		return getEntityInfo(registry, entity);
	}
	return std::nullopt;
}

namespace Actions {

void PatrolAction::execute(Registry &registry, const Entity entity, float deltaTime)
{
	Systems::PatrolSystem::updateEntity(registry, entity);
}

void StopMovementAction::execute(Registry &registry, const Entity entity, float deltaTime)
{
	if (const auto vel = registry.tryGet<VelocityComponent>(entity)) {
		vel->get().velocity.x = 0.0f;
	}
	if (const auto acc = registry.tryGet<AccelerationComponent>(entity)) {
		acc->get().acceleration.x = 0.0f;
	}
	if (const auto input = registry.tryGet<InputComponent>(entity)) {
		input->get().horizontalDirection = 0.0f;
	}
}

void FacePlayerAction::execute(Registry &registry, const Entity entity, float deltaTime)
{
	const auto enemyRender = registry.tryGet<RenderComponent>(entity);
	if (!enemyRender)
		return;

	const auto enemy = getEntityInfo(registry, entity);

	if (const auto player = getPlayerInfo(registry); enemy && player) {
		const bool shouldFaceLeft = player->center.x < enemy->center.x;
		enemyRender->get().setFacingLeft(shouldFaceLeft);
	}
}

void StopAndFacePlayerAction::execute(Registry &registry, const Entity entity, float deltaTime)
{
	const auto enemyRender = registry.tryGet<RenderComponent>(entity);
	const auto inputOpt = registry.tryGet<InputComponent>(entity);

	if (!enemyRender || !inputOpt)
		return;

	const auto enemy = getEntityInfo(registry, entity);

	if (const auto player = getPlayerInfo(registry); enemy && player) {
		enemyRender->get().setFacingLeft(player->center.x < enemy->center.x);
		inputOpt->get().horizontalDirection = 0.0f;
	}
}

void TriggerAnimationAction::execute(Registry &registry, const Entity entity, float deltaTime)
{
	const auto animComp = registry.tryGet<AnimationComponent>(entity);
	if (!animComp)
		return;

	if (animComp->get().getAnimation() != AnimationType::None) {
		animComp->get().setAnimation(animationType);
	}
}

void SetStateComponentAction::execute(Registry &registry, const Entity entity, float deltaTime)
{
	if (const auto stateComp = registry.tryGet<StateComponent>(entity)) {
		stateComp->get().state = state;
	}
}

StopAndFaceWithAnimation::StopAndFaceWithAnimation(AnimationType animationType, bool loop)
{
	add<StopMovementAction>();
	add<FacePlayerAction>();
	add<TriggerAnimationAction>(animationType, loop);
}

void SetVelocityAction::execute(Registry &registry, const Entity entity, float deltaTime)
{
	if (const auto vel = registry.tryGet<VelocityComponent>(entity)) {
		vel->get().velocity = velocity;
	}
}

void SetMovementSpeedAction::execute(Registry &registry, const Entity entity, float deltaTime)
{
	if (const auto movement = registry.tryGet<MovementComponent>(entity)) {
		movement->get().maxSpeedX = speed;
	}
}

} // namespace Actions

namespace Transitions {

bool TimerTransition::canTransition(Registry &registry, const Entity entity, float deltaTime) const
{
	const auto sm = registry.tryGet<StateMachineComponent>(entity);
	if (!sm)
		return false;
	return sm->get().timeInState >= duration;
}

bool AnimationFinishedTransition::canTransition(Registry &registry, const Entity entity, float deltaTime) const
{
	const auto animComp = registry.tryGet<AnimationComponent>(entity);
	if (!animComp)
		return true;

	return animComp->get().hasFinished(animComp->get().getAnimation());
}

bool RandomTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	return Utils::Random::getFloat(0.0f, 1.0f) <= chance;
}

bool ProximityEntryTransition::canTransition(Registry &registry, const Entity entity, float deltaTime) const
{
	const auto enemy = getEntityInfo(registry, entity);
	if (!enemy)
		return false;

	const auto player = getPlayerInfo(registry);
	if (!player)
		return false;

	return glm::distance(enemy->center, player->center) <= radius;
}

bool ProximityExitTransition::canTransition(Registry &registry, const Entity entity, float deltaTime) const
{
	const auto enemy = getEntityInfo(registry, entity);
	if (!enemy)
		return false;

	const auto player = getPlayerInfo(registry);
	if (!player)
		return true; // No player = exited

	return glm::distance(enemy->center, player->center) > radius;
}

bool PlayerUnreachableTransition::canTransition(Registry &registry, const Entity entity, float deltaTime) const
{
	const auto enemy = getEntityInfo(registry, entity);
	if (!enemy)
		return false;

	const auto player = getPlayerInfo(registry);
	if (!player)
		return true;
	const float enemyTop = enemy->hitBoxRect.y;
	const float enemyBottom = enemy->hitBoxRect.y + enemy->hitBoxRect.h;
	const float playerTop = player->hitBoxRect.y;
	const float playerBottom = player->hitBoxRect.y + player->hitBoxRect.h;

	return !((playerTop < enemyBottom + maxHeightDifference) && (playerBottom > enemyTop - maxHeightDifference));
}
PlayerGroundedTransition::PlayerGroundedTransition(AI::AIState target)
{
	this->targetState = target;
}

bool PlayerGroundedTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	for (auto playerView = registry.view<PlayerTag, GroundedComponent>(); const auto playerEntity : playerView) {
		return playerView.get<GroundedComponent>(playerEntity).grounded;
	}
	return false;
}

bool PlayerVerticalRangeTransition::canTransition(Registry &registry, const Entity entity, float deltaTime) const
{
	const auto enemy = getEntityInfo(registry, entity);
	const auto player = getPlayerInfo(registry);

	if (!enemy || !player)
		return false;

	const float enemyTop = enemy->hitBoxRect.y;
	const float enemyBottom = enemy->hitBoxRect.y + enemy->hitBoxRect.h;
	const float playerTop = player->hitBoxRect.y;
	const float playerBottom = player->hitBoxRect.y + player->hitBoxRect.h;

	const bool overlap = (playerTop < enemyBottom + maxVerticalDist) && (playerBottom > enemyTop - maxVerticalDist);
	return overlap;
}

} // namespace Transitions

} // namespace GameEngine::ECS
