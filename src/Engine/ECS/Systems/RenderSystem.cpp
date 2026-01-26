#include "RenderSystem.hpp"
#include "Engine/Debug/ScopedProfiler.hpp"
#include "Engine/ECS/Components/Core/LevelComponent.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Render/BackgroundComponent.hpp"
#include "Engine/ECS/Components/Render/CenterCameraComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/Components/Render/VignetteComponent.hpp"
#include "Engine/ECS/Components/Tags/AlreadySeenTag.hpp"
#include "Engine/ECS/Components/Tags/InactiveTag.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/Utils/RenderUtils.hpp"

#include <execution>
#include <stdexcept>
#include <vector>

using namespace GameEngine::ECS::Systems;
using namespace GameEngine::Utils;

RenderSystem::RenderSystem(Renderer &renderer) : renderer(renderer) {}

void RenderSystem::update(Registry &registry)
{
	{
		PROFILE_SCOPE("RenderSystem::Collect");

		static int renderItemsCapacity = 0;

		renderItems.clear();
		centerCamera(registry);

		auto bgView = registry.view<BackgroundComponent, RenderComponent, PositionComponent>().exclude<InactiveTag>();
		auto view = registry.view<RenderComponent, PositionComponent, SpriteComponent>().exclude<InactiveTag>();

		if (const int requiredCapacity = bgView.size() + view.size(); requiredCapacity > renderItemsCapacity) {
			renderItemsCapacity = static_cast<int>(requiredCapacity * 1.5f);
			renderItems.reserve(renderItemsCapacity);
		}
		// Backgrounds

		for (const auto entity : bgView) {
			const auto bgComp = bgView.get<BackgroundComponent>(entity);
			const auto posComp = bgView.get<PositionComponent>(entity);
			const auto renderComp = bgView.get<RenderComponent>(entity);

			if (const auto item = createBackgroundItem({bgComp, posComp, renderComp}))
				renderItems.push_back(item.value());
		}

		std::vector<Entity> entitiesToCache;
		const SDL_FRect &viewport = renderer.getViewport();
		for (const auto entity : view) {
			const auto &pos = view.get<PositionComponent>(entity);
			const auto &sprite = view.get<SpriteComponent>(entity);
			const auto &render = view.get<RenderComponent>(entity);
			const auto &transform = registry.tryGet<TransformComponent>(entity);

			if (const auto item = RenderUtils::createSpriteItem({pos, sprite, render, transform}, viewport)) {
				renderItems.push_back(item.value());

				if (!registry.has<AlreadySeenTag>(entity)) {
					entitiesToCache.push_back(entity);
				}
			}
		}
		for (auto entity : entitiesToCache) {
			registry.registerComponent(entity, AlreadySeenTag{});
		}
	}

	sortRenderItems(renderItems);
	submitRenderItems(renderItems, renderer);

	for (auto vignetteView = registry.view<VignetteComponent, PositionComponent>(); auto entity : vignetteView) {
		const auto &vignette = vignetteView.get<VignetteComponent>(entity);
		const auto &pos = vignetteView.get<PositionComponent>(entity);

		float intensity = vignette.getIntensity();

		if (intensity > 0.0f) {
			glm::vec2 centerPos = pos.position;

			if (const auto sprite = registry.tryGet<SpriteComponent>(entity)) {
				centerPos.x += sprite->get().tileSize.x / 2.0f;
				centerPos.y += sprite->get().tileSize.y / 2.0f;
			}
			const SDL_FRect viewport = renderer.getViewport();
			glm::vec2 screenPos = centerPos - glm::vec2{viewport.x, viewport.y};

			// 3. Render
			renderer.renderVignette(intensity, vignette.color, screenPos);
		}
	}
	renderItems.clear();
}

void RenderSystem::centerCamera(Registry &registry) const
{

	if (auto centerCameraView =
	        registry.view<CenterCameraComponent, PositionComponent, SpriteComponent>().exclude<InactiveTag>();
	    centerCameraView.size() == 1) {
		const auto &centerCameraEntity = centerCameraView.at(0);
		auto &[position, previousPosition] = centerCameraView.get<PositionComponent>(centerCameraEntity);
		const auto &tile = centerCameraView.get<SpriteComponent>(centerCameraEntity);

		auto levelView = registry.view<LevelComponent, PositionComponent>().exclude<InactiveTag>();

		if (levelView.size() == 1) {
			const auto levelEntity = levelView.at(0);
			const auto &level = levelView.get<LevelComponent>(levelEntity);
			const auto &posLvl = levelView.get<PositionComponent>(levelEntity);
			renderer.centerCameraOn((position + static_cast<glm::vec2>(tile.tileSize / 2)), posLvl.position,
			                        level.levelSize);

			if (const auto render = registry.tryGet<RenderComponent>(levelEntity)) {
				auto &[layer, flipX] = render->get();
				layer = level.playerLayer;
			}

		} else if (levelView.size() > 1) {
			throw std::runtime_error("Level activation error: More than one active level found!");
		}
	} else if (centerCameraView.size() > 1) {
		throw std::runtime_error("There cannot be more than on entity with a CenterCameraComponent");
	}
}

std::optional<RenderUtils::RenderItem> RenderSystem::createBackgroundItem(const BackgroundRenderContext &ctx)
{
	const auto &[layer, flipX] = ctx.renderComp;
	const auto &[position, previousPosition] = ctx.posComp;
	const auto &[texture, scrollSpeed, scrollPos, yOffset, repeatX] = ctx.bgComp;

	constexpr std::optional<SDL_FRect> srcOpt;
	std::optional<SDL_FRect> dstOpt;

	if (repeatX) {
		dstOpt = SDL_FRect{.x = position.x, .y = yOffset + position.y, .h = static_cast<float>(texture->h)};
	}
	return RenderItem{.layer = layer,
	                  .texture = texture,
	                  .src = srcOpt,
	                  .dst = dstOpt,
	                  .parallax = repeatX,
	                  .scrollFactor = scrollSpeed,
	                  .flipX = flipX};
}
