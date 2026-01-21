#include "RenderSystem.hpp"
#include "Engine/DebugSystem.hpp"
#include "Engine/ECS/Components/Core/LevelComponent.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HealthComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/RangedAttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Render/BackgroundComponent.hpp"
#include "Engine/ECS/Components/Render/BlinkComponent.hpp"
#include "Engine/ECS/Components/Render/CenterCameraComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/Components/Tags/InactiveTag.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/Utils/BlinkUtils.hpp"

#include <algorithm>
#include <stdexcept>
#include <vector>

using namespace GameEngine::ECS::Systems;

RenderSystem::RenderSystem(Renderer &renderer) : renderer(renderer) {}

void RenderSystem::update(Registry &registry, float alpha)
{
	renderItems.clear();
	centerCamera(registry);

	// Backgrounds

	for (auto bgView = registry.view<RenderComponent, BackgroundComponent, PositionComponent>().exclude<InactiveTag>();
	     const auto entity : bgView) {
		const auto &[layer, flipX] = bgView.get<RenderComponent>(entity);
		const auto &[position, previousPosition] = bgView.get<PositionComponent>(entity);
		const auto &bg = bgView.get<BackgroundComponent>(entity);

		constexpr std::optional<SDL_FRect> srcOpt;
		std::optional<SDL_FRect> dstOpt;

		if (bg.repeatX) {
			dstOpt = SDL_FRect{.x = position.x, .y = bg.yOffset + position.y, .h = static_cast<float>(bg.texture->h)};
		}

		renderItems.push_back(RenderItem{.layer = layer,
		                                 .texture = bg.texture,
		                                 .src = srcOpt,
		                                 .dst = dstOpt,
		                                 .parallax = bg.repeatX,
		                                 .scrollFactor = bg.scrollingSpeed,
		                                 .flipX = flipX});
	}

	// Tiles, Player
	for (auto view = registry.view<PositionComponent, SpriteComponent, RenderComponent>().exclude<InactiveTag>();
	     const auto entity : view) {
		const auto &[position, previousPosition] = view.get<PositionComponent>(entity);
		const auto &[texture, TileSize, TileMapPosition] = view.get<SpriteComponent>(entity);
		const auto &[layer, flipX] = view.get<RenderComponent>(entity);

		glm::vec2 scale = glm::vec2(1.0f, 1.0f);
		double rotation = 0.0;
		if (registry.has<TransformComponent>(entity)) {
			const auto &transform = registry.get<TransformComponent>(entity);
			scale = transform.scale;
			rotation = transform.rotation;
		}

		const SDL_FRect src{.x = static_cast<float>(TileMapPosition.x),
		                    .y = static_cast<float>(TileMapPosition.y),
		                    .w = static_cast<float>(TileSize.x),
		                    .h = static_cast<float>(TileSize.y)};

		// non interpolation
		SDL_FRect dst{.x = position.x,
		              .y = position.y,
		              .w = static_cast<float>(TileSize.x) * scale.x,
		              .h = static_cast<float>(TileSize.y) * scale.y};

		// interpolation
		/*SDL_FRect dst{.x = pos.previousPosition.x * (1 - alpha) + pos.position.x * alpha,
		              .y = pos.previousPosition.y * (1 - alpha) + pos.position.y * alpha,
		              .w = tile.TileSize,
		              .h = tile.TileSize};*/

		SDL_Color colorMod = {255, 255, 255, 255};
		if (registry.has<BlinkComponent>(entity) && registry.has<HealthComponent>(entity)) {
			colorMod = Utils::Blink::getBlinkColor(registry, entity);
		}

		renderItems.push_back(RenderItem{.layer = layer,
		                                 .texture = texture,
		                                 .src = src,
		                                 .dst = dst,
		                                 .parallax = false,
		                                 .flipX = flipX,
		                                 .angle = rotation,
		                                 .colorMod = colorMod});

		if (DebugSystem::instance().getRenderOptions().showBoundingBoxes && registry.has<CollisionComponent>(entity)) {
			auto col = registry.get<CollisionComponent>(entity).collisionBox;
			col.x = position.x + col.x;
			col.y = position.y + col.y;

			const SDL_Color color =
			    registry.has<MovementComponent>(entity) ? SDL_Color{0, 0, 255, 255} : SDL_Color{255, 0, 0, 255};
			DebugSystem::addRect(col, color);
		}

		if (DebugSystem::instance().getRenderOptions().showHitBoxes
		    && (registry.has<HitBoxComponent>(entity) || registry.has<HurtBoxComponent>(entity))) {
			const auto &[bodyHitbox, weaponHitBox] = registry.get<HitBoxComponent>(entity);
			const auto &[hurtBox] = registry.get<HurtBoxComponent>(entity);
			const bool facingLeft = flipX;

			SDL_FRect hitBoxRect = {0, 0, weaponHitBox.w * scale.x, weaponHitBox.h * scale.y};
			if (facingLeft) {
				hitBoxRect.x = position.x + hurtBox.x * scale.x - weaponHitBox.w * scale.x;
			} else {
				hitBoxRect.x = position.x + hurtBox.x * scale.x + hurtBox.w * scale.x;
			}
			hitBoxRect.y = position.y + hurtBox.y * scale.y;

			if (hitBoxRect.w > 0.0f && hitBoxRect.h > 0.0f) {
				constexpr SDL_Color hitboxColor = {0, 255, 0, 255};
				DebugSystem::addRect(hitBoxRect, hitboxColor);
			}

			SDL_FRect hurtBoxRect = {position.x + hurtBox.x * scale.x, position.y + hurtBox.y * scale.y,
			                         hurtBox.w * scale.x, hurtBox.h * scale.y};
			constexpr SDL_Color hurtBoxColor = {255, 0, 255, 255};
			DebugSystem::addRect(hurtBoxRect, hurtBoxColor);
			if (!registry.has<RangedAttackComponent>(entity) && registry.has<ChaseComponent>(entity)) {
				const auto &chase = registry.get<ChaseComponent>(entity);
				const auto &patrol = registry.get<PatrolComponent>(entity);

				SDL_FRect rangeRect;
				rangeRect.w = patrol.maxBounds.x - patrol.minBounds.x;
				rangeRect.h = chase.detectionHeightRange * 2.0f;
				rangeRect.x = patrol.minBounds.x;
				rangeRect.y = patrol.minBounds.y - chase.detectionHeightRange;

				constexpr SDL_Color rangeColor = {255, 165, 0, 255}; // Orange
				DebugSystem::addRect(rangeRect, rangeColor);
			}

			if (registry.has<RangedAttackComponent>(entity)) {
				const auto &chase = registry.get<ChaseComponent>(entity);

				const float centerX = position.x + hurtBox.x * scale.x + hurtBox.w * scale.x * 0.5f;
				const float centerY = position.y + hurtBox.y * scale.y + hurtBox.h * scale.y * 0.5f;

				SDL_FRect rangeRect;
				const auto detectionWidth = chase.detectionWidthRange.value_or(0.0f);
				rangeRect.w = detectionWidth * 2.0f;
				rangeRect.h = chase.detectionHeightRange * 2.0f;
				rangeRect.x = centerX - detectionWidth;
				rangeRect.y = centerY - chase.detectionHeightRange;

				constexpr SDL_Color rangeColor = {255, 165, 0, 255}; // Orange
				DebugSystem::addRect(rangeRect, rangeColor);
			}
		}
	}

	std::ranges::sort(renderItems, [](const RenderItem &a, const RenderItem &b) { return a.layer < b.layer; });

	for (auto &renderItem : renderItems) {
		SDL_SetTextureColorMod(renderItem.texture, renderItem.colorMod.r, renderItem.colorMod.g, renderItem.colorMod.b);

		if (!renderItem.parallax) {
			renderer.renderTexture(*renderItem.texture, renderItem.src, renderItem.dst, renderItem.flipX,
			                       renderItem.angle);
		} else if (renderItem.dst.has_value()) {
			renderer.renderTextureTiled(*renderItem.texture, renderItem.dst.value(), renderItem.scrollFactor);
		}

		SDL_SetTextureColorMod(renderItem.texture, 255, 255, 255);
	}
	renderItems.clear();
}

void RenderSystem::centerCamera(Registry &registry) const
{

	if (auto centerCameraView =
	        registry.view<PositionComponent, CenterCameraComponent, SpriteComponent>().exclude<InactiveTag>();
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

			if (registry.has<RenderComponent>(levelEntity)) {
				auto &[layer, flipX] = registry.get<RenderComponent>(levelEntity);
				layer = level.playerLayer;
			}

		} else if (levelView.size() > 1) {
			throw std::runtime_error("Level activation error: More than one active level found!");
		}
	} else if (centerCameraView.size() > 1) {
		throw std::runtime_error("There cannot be more than on entity with a CenterCameraComponent");
	}
}
