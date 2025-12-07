#pragma once
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Physics/AccelerationComponent.hpp"
#include "Engine/ECS/Components/Render/BlinkComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/CombatSystem/AttackSystem.hpp"
#include "Engine/ECS/View.hpp"

#include <SDL3/SDL_rect.h>
#include <vector>

namespace GameEngine::ECS::Systems {
struct HurtBoxSystem {
	static void update(Registry &registry, float deltaTime)
	{
		checkEnemyDetection(registry, deltaTime);
	}

  private:
	static void checkEnemyDetection(Registry &registry, const float deltaTime)
	{
		auto enemyView =
		    registry.view<EnemyTag, HurtBoxComponent, PositionComponent, StateComponent, AttackComponent>();

		std::vector<Entity> idleEnemies;
		for (const auto enemyEntity : enemyView) {
			const auto &[state] = registry.get<StateComponent>(enemyEntity);
			const auto &attackComp = registry.get<AttackComponent>(enemyEntity);
			if (state != State::Attacking) {
				idleEnemies.push_back(enemyEntity);
			}
		}

		auto playerView = registry.view<PlayerTag, HurtBoxComponent, PositionComponent, BlinkComponent>();

		for (const auto enemyEntity : idleEnemies) {
			const auto &[enemyPosition, _] = registry.get<PositionComponent>(enemyEntity);
			const auto &[enemyCombatCollisionBox] =
			    registry.get<HurtBoxComponent>(enemyEntity);

			SDL_FRect enemyCombatBox{enemyPosition.x + enemyCombatCollisionBox.x,
			                         enemyPosition.y + enemyCombatCollisionBox.y, enemyCombatCollisionBox.w,
			                         enemyCombatCollisionBox.h};

			for (const auto playerEntity : playerView) {
				auto &[playerPosition, _] = registry.get<PositionComponent>(playerEntity);
				const auto &[playerCombatCollisionBox] =
				    registry.get<HurtBoxComponent>(playerEntity);

				SDL_FRect playerBox{playerPosition.x + playerCombatCollisionBox.x,
				                    playerPosition.y + playerCombatCollisionBox.y, playerCombatCollisionBox.w,
				                    playerCombatCollisionBox.h};

				if (SDL_HasRectIntersectionFloat(&enemyCombatBox, &playerBox)) {
					auto &[state] = registry.get<StateComponent>(enemyEntity);
					state = State::Attacking;

					auto &attack = registry.get<AttackComponent>(enemyEntity);
					AttackSystem::triggerAttack(attack, deltaTime);
				}
			}
		}
	}
};
} // namespace GameEngine::ECS::Systems
