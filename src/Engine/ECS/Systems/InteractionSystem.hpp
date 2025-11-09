#pragma once
#include "Engine/ECS/Components/Core/PortalComponent.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InteractionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InventoryComponent.hpp"
#include "Engine/ECS/Components/Gameplay/KeyComponent.hpp"
#include "Engine/ECS/Components/Gameplay/LockComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/GameEngine.hpp"

namespace GameEngine::ECS::Systems {

class InteractionSystem {
  public:
	static void update(GameEngine &game, Registry &registry)
	{

		for (auto playerView =
		         registry.view<InputComponent, PositionComponent, InteractionComponent, InventoryComponent>();
		     const auto &entity : playerView) {
			const auto &pInput = playerView.get<InputComponent>(entity);
			const auto &[playerPosition, _] = playerView.get<PositionComponent>(entity);
			const auto &[interactionBox] = playerView.get<InteractionComponent>(entity);
			auto &[inventory] = playerView.get<InventoryComponent>(entity);

			if (!pInput.interactPressed) {
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
					if (registry.has<PortalComponent>(interactableObject)) {
						// TransitionLevel(registry, interactableObject, transitionData, game);
					}
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
				}
			}
		}
	}

  private:
	// static void TransitionLevel(Registry &registry, Entity &interactableObject,
	//                             Systems::LevelTransitionData &transitionData, GameEngine &game)
	// {
	// 	auto &portal = registry.get<PortalComponent>(interactableObject);
	//
	// 	const LevelHistory oldHistory = transitionData.history;
	//
	// 	transitionData.history.levelName = transitionData.currentLevel;
	// 	transitionData.history.spawnPoint = portal.portalId;
	// 	if (portal.targetLevel == "previous") {
	// 		game.PushState(
	// 		    std::make_unique<::GameEngine::TransitionLevelState>(oldHistory.levelName, oldHistory.spawnPoint));
	// 	} else {
	// 		game.PushState(
	// 		    std::make_unique<::GameEngine::TransitionLevelState>(portal.targetLevel, portal.targetPortalId));
	// 	}
	// }
};
} // namespace GameEngine::ECS::Systems