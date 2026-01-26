#include "PoiseSystem.hpp"
#include "Engine/Debug/ScopedProfiler.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/PoiseComponent.hpp"
#include "Engine/ECS/View.hpp"

namespace GameEngine::ECS::Systems::PoiseSystem {

void update(Registry &registry, float deltaTime)
{
	PROFILE_SCOPE("PoiseSystem");

	for (auto view = registry.view<PoiseComponent>(); const auto entity : view) {
		auto &poise = view.get<PoiseComponent>(entity);

		if (poise.currentPoise > 0.0f) {
			poise.recoveryTimer.step(std::chrono::duration<float>(deltaTime), false);
			if (poise.recoveryTimer.isTimeout()) {
				poise.currentPoise = 0.0f;
			}
		}
	}
}

} // namespace GameEngine::ECS::Systems::PoiseSystem
