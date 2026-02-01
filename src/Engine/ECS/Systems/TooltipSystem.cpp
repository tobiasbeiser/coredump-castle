#include "TooltipSystem.hpp"

#include "Engine/Debug/DebugSystem.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InteractionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/TooltipComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/ECS/Components/Render/TextDisplayComponent.hpp"

#include <SDL3/SDL_log.h>

void GameEngine::ECS::Systems::TooltipSystem::update(Registry &registry, const float deltaTime)
{
	auto playerView = registry.view<PlayerTag, InteractionComponent, PositionComponent>();
	if (playerView.size() == 0) {
		return; // player is dead
	}

	for (auto view = registry.view<TooltipComponent>(); const auto entity : view) {
		auto &[text, box] = view.get<TooltipComponent>(entity);

		const auto &playerHitbox = playerView.get<InteractionComponent>(playerView.at(0)).interactionBox;
		const auto &playerPosition = playerView.get<PositionComponent>(playerView.at(0)).position;

		SDL_FRect playerInteractionBox{playerPosition.x + playerHitbox.x, playerPosition.y + playerHitbox.y,
		                               playerHitbox.w, playerHitbox.h};

		if (SDL_HasRectIntersectionFloat(&playerInteractionBox, &box)) {
			if (!registry.has<TextDisplayComponent>(entity)) {
				registry.registerComponent(entity, TextDisplayComponent{});
			}
			auto &textDisp = registry.get<TextDisplayComponent>(entity);
			if (textDisp.typewriterTimer.isTimeout() && textDisp.counter < text.size()) {
				textDisp.counter++;
				textDisp.typewriterTimer.reset();
			} else {
				textDisp.typewriterTimer.step(std::chrono::duration<float>(deltaTime), true);
			}
			DebugSystem::debug("Tooltip: " + text);
		} else if (registry.has<TextDisplayComponent>(entity)) {
			registry.remove<TextDisplayComponent>(entity);
		}

	}
}