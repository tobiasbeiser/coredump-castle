
#include "Engine/ECS/Systems/MapTrackingSystem.hpp"

#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/ECS/Components/Core/LevelComponent.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/Components/Tags/AlreadySeenTag.hpp"
#include "Engine/ECS/Components/Tags/InactiveTag.hpp"
#include "Engine/ECS/Components/Tags/PendingReviveTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Components/Tags/UiElementTag.hpp"
#include "Engine/Utils/RenderUtils.hpp"


void GameEngine::ECS::Systems::MapTrackingSystem::update(Registry &registry, const SDL_FRect &viewport)
{
	if (const auto levelView = registry.view<LevelComponent, PositionComponent>().exclude<InactiveTag>(); levelView.size() != 1) {
		return;
	}

	// prevent death frame from being tagged as seen
	for (auto playerView = registry.view<PlayerTag>(); const auto entity : playerView) {
		if (registry.has<PendingReviveTag>(entity)) return;
	}
  
	for (auto view = registry.view<RenderComponent, PositionComponent, SpriteComponent>().exclude<InactiveTag>().exclude<UiElementTag>(); const auto entity : view) {
		const auto &pos = view.get<PositionComponent>(entity);
		const auto &sprite = view.get<SpriteComponent>(entity);
		const auto &transform = registry.tryGet<TransformComponent>(entity);

		const SDL_FRect dst = Utils::RenderUtils::getTileDstFRect({pos, sprite, transform});

		if (!registry.has<AlreadySeenTag>(entity) && SDL_HasRectIntersectionFloat(&dst, &viewport)) {
			registry.registerComponent(entity, AlreadySeenTag{});
		}
	}


}



