#include "MapRenderSystem.hpp"

#include "Engine/ECS/Components/Core/LevelComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/ProjectileComponent.hpp"
#include "Engine/ECS/Components/Tags/InactiveTag.hpp"
#include "Engine/Utils/RenderUtils.hpp"
#include "Game/States/MapMenuState.hpp"

using namespace GameEngine::ECS::Systems;

void MapRenderSystem::update(Registry &registry, GameEngine &game)
{
	Utils::RenderUtils::withClipRect(game.getRenderer().getNativeRenderer(), clipRect, [&] {
		renderItems.clear();
		centerCamera(registry);

		auto view = registry.view<AlreadySeenTag, RenderComponent, PositionComponent, SpriteComponent>()
		                .exclude<EnemyTag>()
		                .exclude<ProjectileComponent>();

		renderItems.reserve(view.size());
		const SDL_FRect &viewport = renderer.getViewport();

		for (const auto entity : view) {
			const auto &pos = view.get<PositionComponent>(entity);
			const auto &sprite = view.get<SpriteComponent>(entity);
			const auto &render = view.get<RenderComponent>(entity);
			const auto &transform = registry.tryGet<TransformComponent>(entity);

			if (const auto item =
			        Utils::RenderUtils::createSpriteItem({pos, sprite, render, transform}, viewport, scalingFactor)) {
				auto renderItem = item.value();
				if (registry.has<InactiveTag>(entity))
					setColorMod(renderItem, invalidColorMod);
				renderItems.push_back(renderItem);
			}
		}

		Utils::RenderUtils::sortRenderItems(renderItems);
		Utils::RenderUtils::submitRenderItems(renderItems, renderer);
	});
}

void MapRenderSystem::centerCamera(Registry &registry) const
{
	auto playerView = registry.view<PlayerTag, PositionComponent, SpriteComponent>();
	if (playerView.size() == 1) {
		const auto playerEntity = playerView.at(0);
		const auto &playerPos = playerView.get<PositionComponent>(playerEntity);
		const auto &tile = playerView.get<SpriteComponent>(playerEntity);
		renderer.setCameraCenter(Utils::RenderUtils::getScaledVec(
		    playerPos.position + static_cast<glm::vec2>(tile.tileSize / 2), scalingFactor));
	}
}

void MapRenderSystem::setScalingFactor(const float newScalingFactor)
{
	scalingFactor = newScalingFactor;
}

void MapRenderSystem::setMapClipRect(const SDL_FRect newClipRect)
{
	clipRect = Utils::RenderUtils::rectFromFRect(newClipRect);
}

float MapRenderSystem::getMapSizeFactor()
{
	return mapSizeFactor;
}
