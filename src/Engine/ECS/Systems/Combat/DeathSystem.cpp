#include "DeathSystem.hpp"

#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HealthComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Render/AnimationComponent.hpp"
#include "Engine/ECS/Components/Render/BlinkComponent.hpp"
#include "Engine/ECS/Components/Render/SimpleDeathComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/View.hpp"
#include "HealthSystem.hpp"

#include <SDL3/SDL_log.h>

namespace {
void removeComponents(GameEngine::ECS::Registry &registry, const std::vector<unsigned>::value_type entity)
{
	registry.remove<VelocityComponent>(entity);
	registry.remove<InputComponent>(entity);
	registry.remove<HurtBoxComponent>(entity);
	registry.remove<BlinkComponent>(entity);
}

void processSimpleDeathAnimation(GameEngine::ECS::Registry &registry, const float deltaTime,
                                 const std::vector<unsigned>::value_type entity)
{
	auto &[deathTimer] = registry.get<SimpleDeathComponent>(entity);
	auto &sprite = registry.get<SpriteComponent>(entity);
	deathTimer.step(std::chrono::duration<float>(deltaTime), false);

	const float timeRatio = deathTimer.getTime() / deathTimer.getLength();
	float alphaVal = 255.0f * (1.0f - timeRatio);

	if (alphaVal < 0)
		alphaVal = 0;
	sprite.colorMod.a = static_cast<uint8_t>(alphaVal);

	if (deathTimer.isTimeout()) {
		registry.deleteEntity(entity);
	}
}

} // namespace

void GameEngine::ECS::Systems::DeathSystem::update(Registry &registry, const float deltaTime)
{
	for (auto view = registry.view<HealthComponent>(); const auto entity : view) {

		if (auto &health = view.get<HealthComponent>(entity); HealthSystem::isDead(health)) {

			removeComponents(registry, entity);

			if (registry.has<SimpleDeathComponent>(entity)) {
				processSimpleDeathAnimation(registry, deltaTime, entity);
				continue;
			}

			if (registry.has<AnimationComponent>(entity)) {
				if (auto &anim = registry.get<AnimationComponent>(entity);
				    anim.hasAnimation(AnimationType::Dead) && !anim.hasFinished(AnimationType::Dead)) {
					continue;
				} else if (!anim.hasAnimation(AnimationType::Dead)) {
					registry.registerComponent<SimpleDeathComponent>(entity, {});
					registry.remove<AnimationComponent>(entity); // freeze current frame
					continue;
				}
			}

			registry.deleteEntity(entity);
		}
	}
}