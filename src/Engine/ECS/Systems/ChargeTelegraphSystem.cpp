#include "ChargeTelegraphSystem.hpp"

#include "Engine/ECS/Components/Render/ChargeTelegraphComponent.hpp"
#include "Engine/ECS/Components/Render/OutlineGlowComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"
#include <cmath>

void GameEngine::ECS::Systems::ChargeTelegraphSystem::update(Registry &registry, float deltaTime)
{
	for (auto view = registry.view<ChargeTelegraphComponent, SpriteComponent>(); const auto entity : view) {
		auto &telegraph = view.get<ChargeTelegraphComponent>(entity);
		auto &sprite = view.get<SpriteComponent>(entity);

		if (!telegraph.active) {
			// Ensure outline glow is disabled when telegraph is inactive
			if (telegraph.useOutlineGlow) {
				if (auto outlineGlowOpt = registry.tryGet<OutlineGlowComponent>(entity)) {
					outlineGlowOpt->get().active = false;
				}
			}
			continue;
		}

		// Step the timer to progress it
		telegraph.telegraphTimer.step(std::chrono::duration<float>(deltaTime), false);

		const auto elapsed =
		    std::chrono::duration_cast<std::chrono::duration<float>>(telegraph.telegraphTimer.getTime()).count();

		if (telegraph.isFinished()) {
			// Reset when finished
			sprite.colorMod = {255, 255, 255, 255};
			telegraph.active = false;
			
			if (telegraph.useOutlineGlow) {
				if (auto outlineGlowOpt = registry.tryGet<OutlineGlowComponent>(entity)) {
					outlineGlowOpt->get().active = false;
				}
			}
			continue;
		}

		// Create a pulsing glow effect using sine wave
		// This creates a smooth intensity oscillation between normal and glow color
		const float pulsePhase = elapsed / telegraph.pulseFrequency;
		const float intensity = (std::sin(pulsePhase * 2.0f * 3.14159f) + 1.0f) / 2.0f; // 0.0 to 1.0

		if (telegraph.useOutlineGlow) {
			// Use outline glow effect (better for reddish sprites like the boss)
			auto outlineGlowOpt = registry.tryGet<OutlineGlowComponent>(entity);
			
			if (!outlineGlowOpt) {
				// Add the component if it doesn't exist
				registry.registerComponent<OutlineGlowComponent>(entity, OutlineGlowComponent{});
				outlineGlowOpt = registry.tryGet<OutlineGlowComponent>(entity);
			}
			
			auto &outlineGlow = outlineGlowOpt->get();
			outlineGlow.active = true;
			outlineGlow.outlineColor = telegraph.glowColor;
			outlineGlow.thickness = telegraph.outlineThickness;
			outlineGlow.intensity = intensity;
		} else {
			// Use color mod effect (original behavior)
			// Lerp between white and glow color based on intensity
			const auto lerpColor = [](Uint8 a, Uint8 b, float t) -> Uint8 {
				return static_cast<Uint8>(a + (b - a) * t);
			};

			sprite.colorMod = {
			    lerpColor(255, telegraph.glowColor.r, intensity),
			    lerpColor(255, telegraph.glowColor.g, intensity),
			    lerpColor(255, telegraph.glowColor.b, intensity),
			    telegraph.glowColor.a
			};
		}
	}
}
