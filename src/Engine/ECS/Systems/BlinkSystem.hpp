#pragma once
#include "Engine/ECS/Components/Render/BlinkComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"

namespace GameEngine::ECS::Systems {
struct BlinkSystem {
	static void update(Registry &registry, const float deltaTime)
	{
		for (auto view = registry.view<BlinkComponent>(); const auto entity : view) {
			auto &blink = registry.get<BlinkComponent>(entity);
			blink.timer.step(deltaTime, false);
		}
	}

	static void applyBlinkEffect(Registry &registry)
	{
		for (auto view = registry.view<BlinkComponent, SpriteComponent>(); const auto entity : view) {
			auto &[timer, color, blinkFrequency] = view.get<BlinkComponent>(entity);
			const auto &sprite = view.get<SpriteComponent>(entity);

			if (!timer.isTimeout() && timer.getTime() > 0) {
				if (const int toggle = static_cast<int>(timer.getTime() / blinkFrequency) % 2; toggle == 0) {
					SDL_SetTextureColorMod(sprite.texture, color.r, color.g, color.b);
				} else {
					resetTextureColor(sprite);
				}
			} else {
				resetTextureColor(sprite);
			}
		}
	}

  private:
	static void resetTextureColor(const SpriteComponent &sprite)
	{
		SDL_SetTextureColorMod(sprite.texture, 255, 255, 255);
	}
};
} // namespace GameEngine::ECS::Systems
