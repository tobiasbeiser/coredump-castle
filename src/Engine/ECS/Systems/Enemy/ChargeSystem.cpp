#include "ChargeSystem.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChargeComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Systems/CollisionSystem.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/Utils/DetectionRangeUtils.hpp"
#include "Engine/Utils/HitBoxUtils.hpp"

namespace GameEngine::ECS::Systems::ChargeSystem {

namespace {

std::optional<StateComponent> getPlayerContext(Registry &registry)
{
	for (auto view =
	         registry.view<PlayerTag, PositionComponent, HurtBoxComponent, TransformComponent, StateComponent>();
	     const auto entity : view) {
		const auto &state = view.get<StateComponent>(entity);

		return state;
	}
	return std::nullopt;
}

void endCharge(Registry &registry, const Entity entity, ChargeComponent &charge, ChaseComponent *chase,
               MovementComponent *movement, InputComponent *input)
{
	charge.chargeTimer.reset();
	charge.chargePauseTimer.reset();
	if (input) {
		input->attackPressed = false;
		input->horizontalDirection = 0.0f;
	}
	if (movement && chase) {
		movement->maxSpeedX = chase->chaseSpeed;
	}
	registry.remove<ChargeComponent>(entity);
}

} // namespace

void updateEntity(Registry &registry, Entity entity, float deltaTime)
{
	if (const auto playerState = getPlayerContext(registry);
	    !playerState.has_value() || playerState->state == State::Dead) {
		auto chargeOpt = registry.tryGet<ChargeComponent>(entity);
		auto chaseOpt = registry.tryGet<ChaseComponent>(entity);
		auto movementOpt = registry.tryGet<MovementComponent>(entity);
		auto inputOpt = registry.tryGet<InputComponent>(entity);

		if (chargeOpt) {
			endCharge(registry, entity, chargeOpt->get(), chaseOpt ? &chaseOpt->get() : nullptr,
			          movementOpt ? &movementOpt->get() : nullptr, inputOpt ? &inputOpt->get() : nullptr);
		}
		return;
	}

	auto chargeOpt = registry.tryGet<ChargeComponent>(entity);
	auto chaseOpt = registry.tryGet<ChaseComponent>(entity);
	auto inputOpt = registry.tryGet<InputComponent>(entity);
	auto movementOpt = registry.tryGet<MovementComponent>(entity);
	auto renderOpt = registry.tryGet<RenderComponent>(entity);
	auto posOpt = registry.tryGet<PositionComponent>(entity);
	auto velOpt = registry.tryGet<VelocityComponent>(entity);

	if (!chargeOpt || !chaseOpt || !inputOpt || !movementOpt || !renderOpt || !posOpt || !velOpt) {
		return;
	}

	auto *charge = &chargeOpt->get();
	auto *chase = &chaseOpt->get();
	auto *input = &inputOpt->get();
	auto *movement = &movementOpt->get();
	auto *render = &renderOpt->get();
	auto *pos = &posOpt->get();
	auto *vel = &velOpt->get();

	auto transform = registry.tryGet<TransformComponent>(entity);
	glm::vec2 scale = transform ? transform->get().scale : glm::vec2{1.0f, 1.0f};

	if (charge->chargePauseTimer.isRunning() && !charge->chargePauseTimer.isTimeout()) {
		charge->chargePauseTimer.step(std::chrono::duration<float>(deltaTime), false);
		input->attackPressed = false;
		input->horizontalDirection = 0.0f;

		if (charge->chargePauseTimer.isTimeout()) {
			movement->maxSpeedX = chase->chaseSpeed;
			endCharge(registry, entity, *charge, chase, movement, input);
		}
		return;
	}



	if (charge->chargeTimer.isRunning() && !charge->chargeTimer.isTimeout()) {
		charge->chargeTimer.step(std::chrono::duration<float>(deltaTime), false);

		float moveDirection = charge->chargeDirection;

		input->attackPressed = false;
		input->horizontalDirection = moveDirection;
		render->setFacingLeft(moveDirection < 0);

		float chargeSpeed = chase->chaseSpeed * charge->chargeSpeedMultiplier;
		movement->maxSpeedX = chargeSpeed;

		if (std::abs(vel->velocity.x) < chargeSpeed) {
			vel->velocity.x = moveDirection * chargeSpeed;
		}

		bool hitWall = false;

		if (std::abs(vel->velocity.x) < 0.1f) {
			hitWall = true;
		}

		if (hitWall) {
			charge->chargeTimer.reset();
			endCharge(registry, entity, *charge, chase, movement, input);
			return;
		}

		auto collisionOpt = registry.tryGet<CollisionComponent>(entity);
		if (collisionOpt) {
			bool canMove = CollisionSystem::canContinueMovingHorizontally(
			    pos->position, collisionOpt->get().collisionBox, scale, moveDirection);

			if (!canMove) {
				charge->chargeTimer.reset();
				vel->velocity.x = 0.0f;
				endCharge(registry, entity, *charge, chase, movement, input);
				return;
			}
		}

		if (charge->chargeTimer.isTimeout()) {
			endCharge(registry, entity, *charge, chase, movement, input);
		}
	}
}

void update(Registry &registry, const float deltaTime)
{
	auto view = registry.view<ChargeComponent, ChaseComponent, InputComponent, MovementComponent, RenderComponent,
	                          PositionComponent, HitBoxComponent, HurtBoxComponent, VelocityComponent>();

	for (const auto entity : view) {
		updateEntity(registry, entity, deltaTime);
	}
}

} // namespace GameEngine::ECS::Systems::ChargeSystem
