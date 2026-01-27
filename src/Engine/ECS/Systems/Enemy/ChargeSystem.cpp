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
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/Utils/DetectionRangeUtils.hpp"
#include "Engine/Utils/HitBoxUtils.hpp"

namespace GameEngine::ECS::Systems::ChargeSystem {

namespace {
struct PlayerContext {
	glm::vec2 center;
	SDL_FRect hurtBox;
	glm::vec2 scale;
	glm::vec2 position;
	const StateComponent *state;
};

struct EnemyContext {
	Entity entity;
	InputComponent *input;
	ChaseComponent *chase;
	MovementComponent *movement;
	RenderComponent *render;
	ChargeComponent *charge;

	SDL_FRect hurtBox;
	glm::vec2 position;
	glm::vec2 scale;
	PositionComponent *posComp;
	VelocityComponent *velComp;
};

float getDirectionToPlayer(const EnemyContext &enemy, const PlayerContext &player)
{
	return player.center.x > enemy.position.x ? 1.0f : -1.0f;
}

std::optional<PlayerContext> getPlayerContext(Registry &registry)
{
	for (auto view =
	         registry.view<PlayerTag, PositionComponent, HurtBoxComponent, TransformComponent, StateComponent>();
	     const auto entity : view) {
		const auto &pos = view.get<PositionComponent>(entity).position;
		const auto &hurtBox = view.get<HurtBoxComponent>(entity).hurtBox;
		const auto &transform = view.get<TransformComponent>(entity);
		const auto &state = view.get<StateComponent>(entity);

		return PlayerContext{Utils::DetectionRangeUtils::calculateCenter(pos, hurtBox, transform.scale), hurtBox,
		                     transform.scale, pos, &state};
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

void update(Registry &registry, const float deltaTime)
{
	const auto playerCtxOpt = getPlayerContext(registry);
	// If no player, we probably shouldn't be charging or should cancel?
	// For now, if no player, let's cancel charge.

	auto view = registry.view<ChargeComponent, ChaseComponent, InputComponent, MovementComponent, RenderComponent,
	                          PositionComponent, HitBoxComponent, HurtBoxComponent, VelocityComponent>();

	for (const auto entity : view) {
		auto &charge = view.get<ChargeComponent>(entity);
		auto &chase = view.get<ChaseComponent>(entity);
		auto &input = view.get<InputComponent>(entity);
		auto &movement = view.get<MovementComponent>(entity);
		auto &render = view.get<RenderComponent>(entity);
		auto &pos = view.get<PositionComponent>(entity);
		auto &vel = view.get<VelocityComponent>(entity);
		const auto &hurtBox =
		    view.get<HitBoxComponent>(entity)
		        .bodyHitBox; // Using body as hurtbox ref for now or the separate hurtbox comp if exists?
		// Note: ChaseSystem used separate HurtBoxComponent.
		// Let's check view args. We included HurtBoxComponent.

		auto &hurtBoxComp = view.get<HurtBoxComponent>(entity);

		auto transform = registry.tryGet<TransformComponent>(entity);
		glm::vec2 scale = transform ? transform->get().scale : glm::vec2{1.0f, 1.0f};

		EnemyContext enemyCtx{entity,       &input, &chase, &movement, &render, &charge, hurtBoxComp.hurtBox,
		                      pos.position, scale,  &pos,   &vel};

		if (!playerCtxOpt.has_value() || playerCtxOpt->state->state == State::Dead) {
			endCharge(registry, entity, charge, &chase, &movement, &input);
			continue;
		}
		const auto &player = playerCtxOpt.value();

		// Update timers
		// Note: ChargeCooldown is updated in ChaseSystem usually (to know when to START).
		// But if we are charging, cooldown doesn't tick? Or does it?
		// Usually cooldown starts AFTER charge.
		// Let's assume cooldown resets on endCharge.

		// Logic from ChaseSystem::updateChargePaused
		const bool isChargePaused = charge.chargePauseTimer.isRunning() && !charge.chargePauseTimer.isTimeout();
		if (isChargePaused) {
			charge.chargePauseTimer.step(std::chrono::duration<float>(deltaTime), false);
			input.attackPressed = false;
			input.horizontalDirection = 0.0f;

			if (charge.chargePauseTimer.isTimeout()) {
				movement.maxSpeedX = chase.chaseSpeed;
				// If pause is done, what happens? Do we charge again? Or is pause POST-charge?
				// Looking at old code: "if (charge.chargePauseTimer.isTimeout()) { enemy.movement->maxSpeedX =
				// chase.chaseSpeed; }" And return true. It seems pause happens... when? In old code, "resetChargeState"
				// resets both timers. "beginCharge" starts chargeTimer. "updateCharging" checks chargeTimer. If
				// collision -> reset chargeTimer, start chargePauseTimer.

				// So: Charge -> Collision -> Pause -> End.
				// OR: Charge -> Timeout -> End.

				// If pause just finished, we are done.
				endCharge(registry, entity, charge, &chase, &movement, &input);
			}
			continue;
		}

		// Logic from ChaseSystem::updateCharging
		const bool isCharging = charge.chargeTimer.isRunning() && !charge.chargeTimer.isTimeout();
		if (isCharging) {
			charge.chargeTimer.step(std::chrono::duration<float>(deltaTime), false);

			const float directionToPlayer = getDirectionToPlayer(enemyCtx, player);
			input.attackPressed = false;
			input.horizontalDirection = directionToPlayer;
			render.setFacingLeft(directionToPlayer < 0);
			movement.maxSpeedX = chase.chaseSpeed * charge.chargeSpeedMultiplier;

			SDL_FRect enemyHurtBoxRect = Utils::HitBoxUtils::getHitBox(pos.position, hurtBoxComp.hurtBox, &scale);
			SDL_FRect playerHurtBoxRect = Utils::HitBoxUtils::getHitBox(player.position, player.hurtBox, &player.scale);

			if (SDL_HasRectIntersectionFloat(&enemyHurtBoxRect, &playerHurtBoxRect)) {
				bool chargingRight = directionToPlayer > 0;
				if (std::abs(vel.velocity.x) > 0.1f) {
					chargingRight = vel.velocity.x > 0;
				}

				if (chargingRight) {
					const float snapPositionX = playerHurtBoxRect.x - enemyHurtBoxRect.w;
					const float diff = snapPositionX - enemyHurtBoxRect.x;
					pos.position.x += diff;
				} else {
					const float snapPositionX = playerHurtBoxRect.x + playerHurtBoxRect.w;
					const float diff = snapPositionX - enemyHurtBoxRect.x;
					pos.position.x += diff;
				}

				vel.velocity.x = 0.0f;

				charge.chargeTimer.reset(); // Stop charging
				charge.chargePauseTimer.reset();
				charge.chargePauseTimer.step(std::chrono::duration<float>(0.0f), false); // Start pause
				input.attackPressed = false;
				input.horizontalDirection = 0.0f;
				// Continue to next frame where isChargePaused will be true
				continue;
			}

			// If timer finished naturally
			if (charge.chargeTimer.isTimeout()) {
				endCharge(registry, entity, charge, &chase, &movement, &input);
			}
		} else {
			// Not charging and not paused? Must be done or weird state.
			endCharge(registry, entity, charge, &chase, &movement, &input);
		}
	}
}

} // namespace GameEngine::ECS::Systems::ChargeSystem
