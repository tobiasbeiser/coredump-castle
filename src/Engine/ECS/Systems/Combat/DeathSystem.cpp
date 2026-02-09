#include "DeathSystem.hpp"

#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/BossComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HealthComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/JumpComponent.hpp"
#include "Engine/ECS/Components/Gameplay/ProgressComponent.hpp"
#include "Engine/ECS/Components/Render/AnimationComponent.hpp"
#include "Engine/ECS/Components/Render/BlinkComponent.hpp"
#include "Engine/ECS/Components/Render/SimpleDeathComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/GameEngine.hpp"
#include "Engine/ECS/View.hpp"
#include "HealthSystem.hpp"
#include "Engine/ECS/Components/Tags/PendingReviveTag.hpp"
#include "Game/Spawners/EnemySpawner.hpp"
#include "Game/States/GameOverState.hpp"

#include <SDL3/SDL_log.h>

namespace {
void removeComponents(GameEngine::ECS::Registry &registry, const std::vector<unsigned>::value_type entity)
{
	registry.remove<VelocityComponent>(entity);
	registry.remove<InputComponent>(entity);
	registry.remove<HurtBoxComponent>(entity);
	registry.remove<BlinkComponent>(entity);
}

bool processSimpleDeathAnimation(SimpleDeathComponent &death, SpriteComponent &sprite, const float deltaTime)
{
	death.deathTimer.step(std::chrono::duration<float>(deltaTime), false);

	const float timeRatio = death.deathTimer.getTime() / death.deathTimer.getLength();
	float alphaVal = 255.0f * (1.0f - timeRatio);

	if (alphaVal < 0)
		alphaVal = 0;
	sprite.colorMod.a = static_cast<uint8_t>(alphaVal);

	if (death.deathTimer.isTimeout()) {
		return true;
	}
	return false;
}

} // namespace

void GameEngine::ECS::Systems::DeathSystem::update(Registry &registry, GameEngine &game, const float deltaTime, std::vector<EnemySpawner> &spawners)
{
	for (auto view = registry.view<HealthComponent>(); const auto entity : view) {

		if (auto &health = view.get<HealthComponent>(entity); HealthSystem::isDead(health)) {

			removeComponents(registry, entity);
			const auto simpleDeathComp = registry.tryGet<SimpleDeathComponent>(entity);
			const auto spriteComp = registry.tryGet<SpriteComponent>(entity);

			if (simpleDeathComp && spriteComp) {
				if (processSimpleDeathAnimation(simpleDeathComp->get(), spriteComp->get(), deltaTime)) {
					registry.deleteEntity(entity);
				}
				continue;
			}

			if (const auto anim = registry.tryGet<AnimationComponent>(entity)) {
				if (anim->get().hasAnimation(AnimationType::Dead) && !anim->get().hasFinished(AnimationType::Dead)) {
					continue;
				}
				if (!anim->get().hasAnimation(AnimationType::Dead)) {
					registry.registerComponent<SimpleDeathComponent>(entity, {});
					registry.remove<AnimationComponent>(entity); // freeze current frame
					continue;
				}
			}

			if (registry.has<PlayerTag>(entity)) {
				registry.registerComponent(entity, PendingReviveTag{});
				game.pushState(std::make_unique<GameOverState>(game.getInputManager(), game.getTextureManager(), game.getGuiManager(), game.getRenderer(), spawners));
			} else {
				registry.deleteEntity(entity);
			}
		}
	}
}