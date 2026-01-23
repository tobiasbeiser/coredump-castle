#include "TooltipSystem.hpp"

#include "Engine/Debug/DebugSystem.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InteractionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/TooltipComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"

#include <SDL3/SDL_log.h>

void GameEngine::ECS::Systems::TooltipSystem::update(Registry &registry)
{
	for (auto view = registry.view<TooltipComponent>(); const auto entity : view) {
		auto &[text, box] = view.get<TooltipComponent>(entity);

		const auto player = registry.view<PlayerTag>().at(0);
		const auto &playerHitbox = registry.get<InteractionComponent>(player).interactionBox;
		const auto &playerPosition = registry.get<PositionComponent>(player).position;

		SDL_FRect playerInteractionBox{playerPosition.x + playerHitbox.x, playerPosition.y + playerHitbox.y,
		                               playerHitbox.w, playerHitbox.h};

		if (SDL_HasRectIntersectionFloat(&playerInteractionBox, &box)) {
			// TODO use GUI Manager to show tooltip
			DebugSystem::debug("Tooltip: " + text);
		}
	}
}