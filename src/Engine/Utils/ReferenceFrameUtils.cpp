//
// Created by leopaetz on 1/25/26.
//

#include "ReferenceFrameUtils.hpp"

#include "Engine/ECS/Components/Core/LevelComponent.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Physics/AccelerationComponent.hpp"
#include "Engine/ECS/Components/Render/CenterCameraComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"

namespace GameEngine::Utils::ReferenceFrameUtils {

bool pointInRect(const glm::vec2 point, const SDL_FRect rect)
{
	const SDL_FPoint checkPoint = {point.x, point.y};
	return SDL_PointInRectFloat(&checkPoint, &rect);
}

bool isInFrame(const glm::vec2 point, const ReferenceFrame &frame)
{
	return point.x >= static_cast<float>(frame.x) && point.y >= static_cast<float>(frame.y)
	       && point.x < static_cast<float>(frame.w + frame.x) && point.y < static_cast<float>(frame.y + frame.h);
}

ReferenceFrame getReferenceFrame(ECS::Registry &registry)
{
	if (auto centerCameraView = registry.view<CenterCameraComponent, PositionComponent, SpriteComponent>();
	    centerCameraView.size() == 1) {
		const auto &centerCameraEntity = centerCameraView.at(0);
		const auto &[position, previousPosition] = centerCameraView.get<PositionComponent>(centerCameraEntity);
		const auto &tile = centerCameraView.get<SpriteComponent>(centerCameraEntity);

		const auto centerCameraPosition = position + static_cast<glm::vec2>(tile.tileSize / 2);
		for (auto levelView = registry.view<LevelComponent, PositionComponent>(); const auto entity : levelView) {
			const auto &[levelSize, playerLayer] = levelView.get<LevelComponent>(entity);

			if (auto &[lvlPosition, _] = levelView.get<PositionComponent>(entity);
			    pointInRect(centerCameraPosition, {lvlPosition.x, lvlPosition.y, levelSize.x, levelSize.y})) {
				return ReferenceFrame(static_cast<int>(lvlPosition.x), static_cast<int>(lvlPosition.y),
				                      static_cast<int>(levelSize.x), static_cast<int>(levelSize.y), playerLayer);
			}
		}
	}
	return {};
}

} // namespace GameEngine::Utils::ReferenceFrameUtils