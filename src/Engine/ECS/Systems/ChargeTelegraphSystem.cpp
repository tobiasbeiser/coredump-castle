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
			if (telegraph.useOutlineGlow) {
				if (const auto outlineGlowOpt = registry.tryGet<OutlineGlowComponent>(entity)) {
					outlineGlowOpt->get().active = false;
				}
			}
			continue;
		}

		telegraph.telegraphTimer.step(std::chrono::duration<float>(deltaTime), false);

		const auto elapsed =
		    std::chrono::duration_cast<std::chrono::duration<float>>(telegraph.telegraphTimer.getTime()).count();

		if (telegraph.isFinished()) {
			sprite.colorMod = {255, 255, 255, 255};
			telegraph.active = false;

			if (telegraph.useOutlineGlow) {
				if (const auto outlineGlowOpt = registry.tryGet<OutlineGlowComponent>(entity)) {
					outlineGlowOpt->get().active = false;
				}
			}
			continue;
		}

		const float pulsePhase = elapsed / telegraph.pulseFrequency;
		const float intensity = (std::sin(pulsePhase * 2.0f * 3.14159f) + 1.0f) / 2.0f;

		if (telegraph.useOutlineGlow) {
			auto outlineGlowOpt = registry.tryGet<OutlineGlowComponent>(entity);

			if (!outlineGlowOpt) {
				registry.registerComponent<OutlineGlowComponent>(entity, OutlineGlowComponent{});
				outlineGlowOpt = registry.tryGet<OutlineGlowComponent>(entity);
			}

			auto &outlineGlow = outlineGlowOpt->get();
			outlineGlow.active = true;
			outlineGlow.outlineColor = telegraph.glowColor;
			outlineGlow.thickness = telegraph.outlineThickness;
			outlineGlow.intensity = intensity;
		} else {
			const auto lerpColor = [](Uint8 a, Uint8 b, float t) -> Uint8 {
				return static_cast<Uint8>(a + (b - a) * t);
			};

			sprite.colorMod = {lerpColor(255, telegraph.glowColor.r, intensity),
			                   lerpColor(255, telegraph.glowColor.g, intensity),
			                   lerpColor(255, telegraph.glowColor.b, intensity), telegraph.glowColor.a};
		}
	}
}
