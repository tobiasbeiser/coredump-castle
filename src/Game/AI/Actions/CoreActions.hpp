#pragma once

#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Render/AnimationComponent.hpp"
#include "Engine/ECS/Components/StateMachine/StateMachineComponent.hpp"
#include "Game/AI/States.hpp"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <optional>
#include <string>

namespace GameEngine::ECS {

namespace Actions {

struct PatrolAction final : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct StopMovementAction final : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct FacePlayerAction final : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct StopAndFacePlayerAction final : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct TriggerAnimationAction final : StateAction {
	AnimationType animationType;
	bool loop;

	explicit TriggerAnimationAction(AnimationType animationType, const bool shouldLoop = false)
	    : animationType(), loop(shouldLoop)
	{
	}

	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct SetStateComponentAction final : StateAction {
	State state;

	explicit SetStateComponentAction(State s) : state(s) {}

	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct StopAndFaceWithAnimation final : CompositeAction {
	explicit StopAndFaceWithAnimation(AnimationType animationType, bool loop = false);
};

struct SetVelocityAction final : StateAction {
	glm::vec2 velocity;

	explicit SetVelocityAction(const glm::vec2 vel) : velocity(vel) {}

	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct SetMovementSpeedAction final : StateAction {
	float speed;

	explicit SetMovementSpeedAction(const float newSpeed) : speed(newSpeed) {}

	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

} // namespace Actions

namespace Transitions {

struct TimerTransition final : Transition {
	float duration;

	TimerTransition(AI::AIState targetState, float durationSeconds) : duration(durationSeconds)
	{
		this->targetState = targetState;
	}

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

struct AnimationFinishedTransition final : Transition {
	explicit AnimationFinishedTransition(const AI::AIState targetState) { this->targetState = targetState; }

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

struct RandomTransition final : Transition {
	float chance;

	RandomTransition(AI::AIState targetState, float transitionChance) : chance(transitionChance)
	{
		this->targetState = targetState;
	}

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

struct ProximityEntryTransition final : Transition {
	float radius;

	ProximityEntryTransition(AI::AIState targetState, float proximityRadius) : radius(proximityRadius)
	{
		this->targetState = targetState;
	}

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

struct ProximityExitTransition : Transition {
	float radius;

	ProximityExitTransition(AI::AIState targetState, float proximityRadius) : radius(proximityRadius)
	{
		this->targetState = targetState;
	}

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

struct PlayerUnreachableTransition final : Transition {
	float maxHeightDifference;

	explicit PlayerUnreachableTransition(const AI::AIState target, const float maxHeight = 32.0f)
	    : maxHeightDifference(maxHeight)
	{
		this->targetState = target;
	}

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

struct PlayerGroundedTransition final : Transition {
	explicit PlayerGroundedTransition(AI::AIState target);

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

struct ImmediateTransition final : Transition {
	explicit ImmediateTransition(const AI::AIState targetState) { this->targetState = targetState; }

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override { return true; }
};

struct PlayerVerticalRangeTransition final : Transition {
	float maxVerticalDist;

	PlayerVerticalRangeTransition(AI::AIState target, float maxDist) : maxVerticalDist(maxDist)
	{
		this->targetState = target;
	}

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

} // namespace Transitions

struct EntityInfo {
	glm::vec2 position{};
	SDL_FRect hitBoxRect{};
	glm::vec2 center{};
	glm::vec2 scale{};
};

std::optional<EntityInfo> getEntityInfo(Registry &registry, Entity entity);
std::optional<EntityInfo> getPlayerInfo(Registry &registry);

} // namespace GameEngine::ECS
