#pragma once
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/CombatCollisionComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include <SDL3/SDL_rect.h>
#include <vector>

namespace GameEngine::ECS::Systems {
class CombatCollisionSystem {
  public:
	static void update(Registry &registry)
	{
		// Reset collision state
		for (const auto entity : registry.view<CombatCollisionComponent>()) {
			auto &combat = registry.get<CombatCollisionComponent>(entity);
			combat.collidedWith = std::nullopt;
			combat.isHit = false;
		}

		checkPlayerEnemyCollisions(registry);

		checkEnemyPlayerCollisions(registry);
	}

  private:
	static void checkPlayerEnemyCollisions(Registry &registry) {}

	static void checkEnemyPlayerCollisions(Registry &registry)
	{
		auto enemyView =
		    registry.view<EnemyTag, CombatCollisionComponent, PositionComponent, StateComponent, AttackComponent>();

		std::vector<Entity> idleEnemies;
		for (const auto enemyEntity : enemyView) {
			const auto &stateComp = registry.get<StateComponent>(enemyEntity);
			const auto &attackComp = registry.get<AttackComponent>(enemyEntity);
			if (stateComp.state != State::Attacking) {
				idleEnemies.push_back(enemyEntity);
			}
		}

		auto playerView = registry.view<PlayerTag, CombatCollisionComponent, PositionComponent>();

		for (const auto enemyEntity : idleEnemies) {
			const auto &[enemyPosition, _] = registry.get<PositionComponent>(enemyEntity);

			auto &enemyState = registry.get<StateComponent>(enemyEntity);

			const auto &[combatCollisionBox, collidedWith, isHit] =
			    registry.get<CombatCollisionComponent>(enemyEntity);

			// Enemy's combat hitbox
			SDL_FRect enemyCombatBox{enemyPosition.x + combatCollisionBox.x, enemyPosition.y + combatCollisionBox.y,
			                         combatCollisionBox.w, combatCollisionBox.h};

			// Check collision with player's hitbox
			for (const auto playerEntity : playerView) {
				const auto &[playerPosition, _] = registry.get<PositionComponent>(playerEntity);
				const auto &[playerCombatCollisionBox, playerCollidedWith, playerIsHit] =
				    registry.get<CombatCollisionComponent>(playerEntity);

				SDL_FRect playerBox{playerPosition.x + playerCombatCollisionBox.x,
				                    playerPosition.y + playerCombatCollisionBox.y, playerCombatCollisionBox.w,
				                    playerCombatCollisionBox.h};

				if (SDL_HasRectIntersectionFloat(&enemyCombatBox, &playerBox)) {
					enemyState.state = State::Attacking;
					// enemy should be not be movalbe if player pushes him
					if (registry.has<VelocityComponent>(enemyEntity)) {
						auto &enemyVelocity = registry.get<VelocityComponent>(enemyEntity);
						enemyVelocity.velocity.x = 0.0f;
						enemyVelocity.velocity.y = 0.0f;
					}
				}
			}
		}
	};

} // namespace GameEngine::ECS::Systems