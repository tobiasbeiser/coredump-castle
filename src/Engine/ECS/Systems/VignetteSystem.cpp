#include "VignetteSystem.hpp"
#include "Engine/ECS/Components/Render/VignetteComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"

#include <SDL3/SDL_log.h>

void GameEngine::ECS::Systems::VignetteSystem::update(Registry &registry, float deltaTime)
{
	for (auto vignetteView = registry.view<VignetteComponent>(); const auto entity : vignetteView) {
		auto &vignetteComp = vignetteView.get<VignetteComponent>(entity);
		SDL_Log("Stepping %0.5f", deltaTime);
		vignetteComp.duration.step(std::chrono::duration<float>(deltaTime), false);
		if (vignetteComp.duration.isTimeout()) {
			SDL_Log("timed out");
			registry.remove<VignetteComponent>(entity);
		}
	}
}
