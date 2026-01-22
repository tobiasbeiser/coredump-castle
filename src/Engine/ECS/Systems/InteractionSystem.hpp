#pragma once
#include "Engine/Debug/ScopedProfiler.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/SavePointComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InteractionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InventoryComponent.hpp"
#include "Engine/ECS/Components/Gameplay/KeyComponent.hpp"
#include "Engine/ECS/Components/Gameplay/LockComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/GameEngine.hpp"
#include "SaveSystem.hpp"

namespace GameEngine::ECS::Systems {

class InteractionSystem {
  public:
	static void update(GameEngine &game, Registry &registry)
	{
		PROFILE_SCOPE("InteractionSystem");

		for (auto playerView = registry.view<InteractionComponent, InputComponent, PositionComponent,
		                                     InventoryComponent, StateComponent>();
		     const auto &entity : playerView) {
			const auto &pInput = playerView.get<InputComponent>(entity);
			const auto &[playerPosition, _] = playerView.get<PositionComponent>(entity);
			const auto &[interactionBox] = playerView.get<InteractionComponent>(entity);
			auto &[inventory] = playerView.get<InventoryComponent>(entity);
			auto &[state] = playerView.get<StateComponent>(entity);

			if (!pInput.interactPressed || state == State::Interacting) {
				return;
			}
			SDL_FRect playerInteractionBox{playerPosition.x + interactionBox.x, playerPosition.y + interactionBox.y,
			                               interactionBox.w, interactionBox.h};

			for (auto interactableObjects = registry.view<PositionComponent, InteractionComponent>();
			     auto &interactableObject : interactableObjects) {
				if (entity == interactableObject) {
					continue;
				}
				const auto &objPos = interactableObjects.get<PositionComponent>(interactableObject);
				const auto &objInt = interactableObjects.get<InteractionComponent>(interactableObject);

				SDL_FRect objInteractionBox{objPos.position.x + objInt.interactionBox.x,
				                            objPos.position.y + objInt.interactionBox.y, objInt.interactionBox.w,
				                            objInt.interactionBox.h};

				if (SDL_HasRectIntersectionFloat(&playerInteractionBox, &objInteractionBox)) {
					state = State::Interacting;
					if (registry.has<KeyComponent>(interactableObject)) {
						const auto [id] = registry.get<KeyComponent>(interactableObject);
						inventory.emplace_back(id);
						registry.deleteEntity(interactableObject);

						for (auto lockView = registry.view<LockComponent>(); const auto lockEntity : lockView) {
							if (const auto &[lockId] = lockView.get<LockComponent>(lockEntity); lockId == id) {
								registry.deleteEntity(lockEntity);
							}
						}
					}
					if (registry.has<SavePointComponent>(interactableObject)) {
						game.getSaveSystem().saveGame(registry);
					}
				}
			}
		}
	}
};
} // namespace GameEngine::ECS::Systems