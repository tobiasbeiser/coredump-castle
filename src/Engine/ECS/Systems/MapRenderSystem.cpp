#include "MapRenderSystem.hpp"

#include "Engine/ECS/Components/Core/LevelComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/ProjectileComponent.hpp"
#include "Engine/ECS/Components/Tags/AlreadySeenTag.hpp"
#include "Engine/ECS/Components/Tags/InactiveTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/Utils/RenderUtils.hpp"
#include "Game/States/MapMenuState.hpp"

using namespace GameEngine::ECS::Systems;

void MapRenderSystem::update(Registry &registry, GameEngine &game)
{
	Utils::RenderUtils::withClipRect(game.getRenderer().getNativeRenderer(), clipRect, [&] {
		renderItems.clear();
		game.getRenderer().setCameraCenter(Utils::RenderUtils::getScaledVec(focusPoint, scalingFactor));

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

void MapRenderSystem::initFocusPoint(Registry &registry)
{
	auto playerView = registry.view<PlayerTag, PositionComponent, SpriteComponent>();
	if (playerView.size() == 1) {
		const auto playerEntity = playerView.at(0);
		const auto &playerPos = playerView.get<PositionComponent>(playerEntity);
		const auto &tile = playerView.get<SpriteComponent>(playerEntity);
		focusPoint = playerPos.position + static_cast<glm::vec2>(tile.tileSize / 2);
	}
}

void MapRenderSystem::setMapClipRect(const SDL_FRect newClipRect)
{
	clipRect = Utils::RenderUtils::rectFromFRect(newClipRect);
}

float MapRenderSystem::getMapSizeFactor()
{
	return mapSizeFactor;
}

void MapRenderSystem::handleZoom(const float wheelAmount)
{
	constexpr float zoomSpeed = 1.1f;
	if (wheelAmount > 0) {
		scalingFactor *= zoomSpeed;
	} else if (wheelAmount < 0) {
		scalingFactor /= zoomSpeed;
	}
	scalingFactor = std::clamp(scalingFactor, 0.1f, 5.0f);
}
