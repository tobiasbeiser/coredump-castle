#pragma once
#include "Engine/ECS/Components/Core/PortalComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InteractionComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/GameEngine.hpp"
#include "Game/States/TransitionLevelState.hpp"
#include <memory>

namespace GameEngine::ECS::Systems {

class InteractionSystem {
  public:
	static void Update(GameEngine &game, Registry &registry)
	{

		auto view = registry.view<InputComponent, PositionComponent, InteractionComponent>();
		for (auto &entity : view) {
			auto &pInput = view.get<InputComponent>(entity);
			auto &pPos = view.get<PositionComponent>(entity);
			auto &pInt = view.get<InteractionComponent>(entity);

			if (!pInput.interactPressed) {
				return;
			}
			SDL_FRect playerInteractionBox{pPos.position.x + pInt.interactionBox.x,
			                               pPos.position.y + pInt.interactionBox.y, pInt.interactionBox.w,
			                               pInt.interactionBox.h};



			auto interactableObjects = registry.view<PositionComponent, InteractionComponent>();

			for (auto &interactableObject : interactableObjects) {
				if (entity == interactableObject) {
					continue;
				}
				auto &objPos = interactableObjects.get<PositionComponent>(interactableObject);
				auto &objInt = interactableObjects.get<InteractionComponent>(interactableObject);

				SDL_FRect objInteractionBox{objPos.position.x + objInt.interactionBox.x,
				                            objPos.position.y + objInt.interactionBox.y, objInt.interactionBox.w,
				                            objInt.interactionBox.h};

				if (SDL_HasRectIntersectionFloat(&playerInteractionBox, &objInteractionBox)) {
					if (registry.has<PortalComponent>(interactableObject)) {
						auto &portal = registry.get<PortalComponent>(interactableObject);

						game.PushState(std::make_unique<::GameEngine::TransitionLevelState>(portal.targetLevel,
						                                                                    portal.targetPortalId));
					}
				}
			}
		}
	}
};
} // namespace GameEngine::ECS::Systems