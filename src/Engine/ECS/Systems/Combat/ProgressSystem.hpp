#pragma once
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Gameplay/BossComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HealthComponent.hpp"
#include "Engine/ECS/Components/Gameplay/JumpComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InteractionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/JumpComponent.hpp"
#include "Engine/ECS/Components/Gameplay/ProgressComponent.hpp"
#include "Engine/ECS/Components/Gameplay/TooltipComponent.hpp"
#include "Engine/ECS/Components/Render/AnimationComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/View.hpp"
#include "HealthSystem.hpp"

namespace GameEngine::ECS::Systems::ProgressSystem {

template <typename T>
concept SaveSystemConcept = requires(const T &t, Registry &r) {
	{ t.saveGame(r) } -> std::same_as<void>;
};

template <SaveSystemConcept TSaveSystem>
void update(Registry &registry, const TSaveSystem &saveSystem)
{
	for (auto bossView = registry.view<BossComponent, HealthComponent>(); const auto entity : bossView) {
		if (auto &health = bossView.get<HealthComponent>(entity); HealthSystem::isDead(health)) {
			const auto &boss = bossView.get<BossComponent>(entity);

			if (auto playerView =
			        registry
			            .view<PlayerTag, PositionComponent, ProgressComponent, InteractionComponent, JumpComponent>();
			    playerView.size() == 1U) {

				const Entity playerEntity = playerView.at(0U);
				auto &progress = playerView.get<ProgressComponent>(playerEntity);
				auto &jump = playerView.get<JumpComponent>(playerEntity);

				if (const int bossId = boss.bossId; !progress.isBossDefeated(bossId)) {
					progress.markBossDefeated(bossId);
					progress.unlockAbility(boss.unlockAbility);

					if (boss.unlockAbility == AbilityType::DoubleJump) {
						jump.maxJumps = std::max(jump.maxJumps, 2);
						jump.jumpsLeft = jump.maxJumps;
					}

					const auto &playerHitbox = playerView.get<InteractionComponent>(playerView.at(0)).interactionBox;
					const auto &playerPosition = playerView.get<PositionComponent>(playerView.at(0)).position;

					SDL_FRect playerInteractionBox{playerPosition.x + playerHitbox.x, playerPosition.y + playerHitbox.y,
					                               playerHitbox.w, playerHitbox.h};

					for (auto view = registry.view<TooltipComponent>(); const auto tipEntity : view) {
						auto &[text, box, enabled] = view.get<TooltipComponent>(tipEntity);
						if (SDL_HasRectIntersectionFloat(&playerInteractionBox, &box)) {
							enabled = true;
						}
					}

					if (const auto state = registry.tryGet<StateComponent>(playerEntity)) {
						state->get().state = State::Interacting;
					}
					if (const auto anim = registry.tryGet<AnimationComponent>(playerEntity);
					    anim && anim->get().hasAnimation(AnimationType::Interacting)
					    && anim->get().getAnimation() != AnimationType::Interacting) {
						anim->get().setAnimation(AnimationType::Interacting);
					}

					saveSystem.saveGame(registry);
				}
			}
		}
	}
}

} // namespace GameEngine::ECS::Systems::ProgressSystem
