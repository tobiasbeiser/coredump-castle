#include "BlinkUtils.hpp"

#include "Engine/ECS/Components/Gameplay/Combat/HealthComponent.hpp"
#include "Engine/ECS/Components/Render/BlinkComponent.hpp"
#include "Engine/ECS/Registry.hpp"

using namespace GameEngine::ECS;

namespace GameEngine::Utils::Blink {

SDL_Color getBlinkColor(Registry &registry, Entity entity)
{
	const auto &health = registry.get<HealthComponent>(entity);
	auto &[color, blinkFrequency] = registry.get<BlinkComponent>(entity);
	const auto elapsed =
	    std::chrono::duration_cast<std::chrono::duration<float>>(health.invulnerabilityTimer.getTime()).count();
	if (!health.invulnerabilityTimer.isTimeout()) {
		if (const int toggle = static_cast<int>(elapsed / blinkFrequency) % 2; toggle != 0) {
			return color;
		}
	}
	return {255, 255, 255, 255};
}

} // namespace GameEngine::Utils::Blink
