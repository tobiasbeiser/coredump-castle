#include "BlinkSystem.hpp"

#include "Engine/ECS/Components/Gameplay/Combat/HealthComponent.hpp"
#include "Engine/ECS/Components/Render/BlinkComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/View.hpp"

#include <SDL3/SDL_log.h>

void GameEngine::ECS::Systems::BlinkSystem::update(Registry &registry)
{
	for (auto view = registry.view<BlinkComponent, HealthComponent, SpriteComponent>(); const auto entity : view) {
		const auto &health = registry.get<HealthComponent>(entity);
		auto &[color, blinkFrequency] = registry.get<BlinkComponent>(entity);
		auto &sprite = view.get<SpriteComponent>(entity);

		const auto elapsed =
		    std::chrono::duration_cast<std::chrono::duration<float>>(health.invulnerabilityTimer.getTime()).count();
		if (!health.invulnerabilityTimer.isTimeout()) {
			if (const int toggle = static_cast<int>(elapsed / blinkFrequency) % 2 < 1; toggle != 0) {
				sprite.colorMod = {color};
				continue;
			}
		}
		sprite.colorMod = {255, 255, 255, 255};
	}
}