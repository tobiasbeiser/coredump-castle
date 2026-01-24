#include "EffectSystem.hpp"

// #include "Engine/Debug/ScopedProfiler.hpp"
#include "Engine/ECS/Components/Render/EffectComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"

void GameEngine::ECS::Systems::EffectSystem::update(Registry &registry, const float deltaTime)
{
	PROFILE_SCOPE("EffectSystem");

	for (auto view = registry.view<EffectComponent, SpriteComponent>(); const auto entity : view) {
		auto &effect = view.get<EffectComponent>(entity);
		auto &[texture, tileSize, tileMapPosition, alpha] = view.get<SpriteComponent>(entity);

		effect.animation.step(std::chrono::duration<float>(deltaTime));

		if (effect.animation.completed() && effect.shouldDestroyOnFinish) {
			registry.deleteEntity(entity);
			continue;
		}

		auto &[frameTexture, frameTileSize, frameTileMapPosition] = effect.frames.at(effect.animation.currentFrame());
		texture = frameTexture;
		tileSize = frameTileSize;
		tileMapPosition = frameTileMapPosition;
	}
}