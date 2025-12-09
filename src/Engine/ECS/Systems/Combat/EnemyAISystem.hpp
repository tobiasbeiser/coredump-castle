#pragma once
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/Combat/AttackSystem.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/Utils/PhysicsUtils.hpp"

#include <SDL3/SDL_rect.h>

namespace GameEngine::ECS::Systems {
struct EnemyAISystem {
	static void update(Registry &registry, const float deltaTime) { checkAttackRange(registry, deltaTime); }

  private:
	static void checkAttackRange(Registry &registry, const float deltaTime)
	{
		auto enemyView = registry.view<EnemyTag, HitBoxComponent, PositionComponent, StateComponent, AttackComponent>();
		auto playerView = registry.view<PlayerTag, HitBoxComponent, PositionComponent>();

		for (const auto enemyEntity : enemyView) {
			auto &[state] = registry.get<StateComponent>(enemyEntity);

			if (state == State::Attacking) {
				continue;
			}

			const auto &[enemyPosition, _] = registry.get<PositionComponent>(enemyEntity);
			const auto &[enemyCombatCollisionBox] = registry.get<HitBoxComponent>(enemyEntity);

			SDL_FRect enemyCombatBox = Utils::getBoundingBox(enemyPosition, enemyCombatCollisionBox);

			for (const auto playerEntity : playerView) {
				auto &[playerPosition, _] = registry.get<PositionComponent>(playerEntity);
				const auto &[playerCombatCollisionBox] = registry.get<HitBoxComponent>(playerEntity);

				SDL_FRect playerBox = Utils::getBoundingBox(playerPosition, playerCombatCollisionBox);

				if (SDL_HasRectIntersectionFloat(&enemyCombatBox, &playerBox)) {
					state = State::Attacking;

					auto &attack = registry.get<AttackComponent>(enemyEntity);
					AttackSystem::triggerAttack(attack, deltaTime);
					break;
				}
			}
		}
	}
};
} // namespace GameEngine::ECS::Systems
