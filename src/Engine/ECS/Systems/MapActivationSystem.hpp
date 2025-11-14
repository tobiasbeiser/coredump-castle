#pragma once
#include "Engine/ECS/Components/Core/LevelComponent.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Render/CenterCameraComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/Components/Tags/InactiveTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"

namespace GameEngine::ECS::Systems {
class MapActivationSystem {

  public:
	void update(Registry &registry)
	{
		if (const auto referenceFrame = getReferenceFrame(registry); referenceFrame != previousReferenceFrame) {
			previousReferenceFrame = referenceFrame;

			// Activate/deactivate entities if they are inside/outside the current level
			for (auto view = registry.view<PositionComponent>().exclude<PlayerTag>(); const auto entity : view) {
				if (auto &[position, previousPosition] = registry.get<PositionComponent>(entity);
				    position.x >= static_cast<float>(referenceFrame.x)
				    && position.y >= static_cast<float>(referenceFrame.y)
				    && position.x < static_cast<float>(referenceFrame.w + referenceFrame.x)
				    && position.y
				           < static_cast<float>(referenceFrame.y + referenceFrame.h)) { // entity is in referenceFrame

					if (registry.has<InactiveTag>(entity)) {
						registry.remove<InactiveTag>(entity);
					}
				} else {
					registry.registerComponent(entity, InactiveTag{});
				}
			}

			// Set player layer correctly
			for (auto playerView = registry.view<PlayerTag, RenderComponent>(); const auto playerEntity : playerView) {
				auto &render = playerView.get<RenderComponent>(playerEntity);
				render.layer = referenceFrame.playerLayer;
			}
		}
	};

  private:
	struct ReferenceFrame {
		int x, y, w, h;
		int playerLayer;
		auto operator<=>(const ReferenceFrame &) const = default;
	};

	ReferenceFrame previousReferenceFrame{};

	static ReferenceFrame getReferenceFrame(Registry &registry)
	{
		if (auto centerCameraView = registry.view<PositionComponent, CenterCameraComponent, SpriteComponent>();
		    centerCameraView.size() == 1) {
			const auto &centerCameraEntity = centerCameraView.at(0);
			const auto &[position, previousPosition] = centerCameraView.get<PositionComponent>(centerCameraEntity);
			const auto &tile = centerCameraView.get<SpriteComponent>(centerCameraEntity);

			const auto centerCameraPosition = position + static_cast<glm::vec2>(tile.tileSize / 2);
			for (auto levelView = registry.view<LevelComponent, PositionComponent>(); const auto entity : levelView) {
				const auto &level = levelView.get<LevelComponent>(entity);
				auto &posLvl = levelView.get<PositionComponent>(entity);

				const bool cameraObjInLevelPos =
				    centerCameraPosition.x >= posLvl.position.x && centerCameraPosition.y >= posLvl.position.y;
				const bool cameraObjInLevelSizeX = centerCameraPosition.x <= posLvl.position.x + level.levelSize.x;
				const bool cameraObjInLevelSizeY = centerCameraPosition.y <= posLvl.position.y + level.levelSize.y;

				if (cameraObjInLevelPos && cameraObjInLevelSizeX && cameraObjInLevelSizeY) {
					return ReferenceFrame(static_cast<int>(posLvl.position.x), static_cast<int>(posLvl.position.y),
					                      static_cast<int>(level.levelSize.x), static_cast<int>(level.levelSize.y),
					                      level.playerLayer);
				}
			}
		}
		return {};
	}
};
} // namespace GameEngine::ECS::Systems