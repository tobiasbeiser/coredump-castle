#pragma once
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/CombatCollisionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/PushbackComponent.hpp"
#include "Engine/ECS/Components/Physics/AccelerationComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"

#include <SDL3/SDL_rect.h>
#include <cmath>

namespace GameEngine::ECS::Systems {
struct PushbackSystem {
	static void update(Registry &registry, float deltaTime)
	{
		// Find all enemies with PushbackComponent (e.g., dumb base enemies)
		auto enemyView =
		    registry
		        .view<EnemyTag, PushbackComponent, CombatCollisionComponent, PositionComponent, VelocityComponent>();

		auto playerView = registry.view<PlayerTag, CombatCollisionComponent, PositionComponent>();

		for (const auto enemyEntity : enemyView) {
			const auto &[enemyPos, _] = registry.get<PositionComponent>(enemyEntity);
			const auto enemyCombat = registry.get<CombatCollisionComponent>(enemyEntity);
			const auto &[pushbackForceY] = registry.get<PushbackComponent>(enemyEntity);

			SDL_FRect enemyBox{enemyPos.x + enemyCombat.hitbox.x, enemyPos.y + enemyCombat.hitbox.y,
			                   enemyCombat.hitbox.w, enemyCombat.hitbox.h};

			for (const auto playerEntity : playerView) {
				auto &[playerPos, _] = registry.get<PositionComponent>(playerEntity);
				const auto playerCombat = registry.get<CombatCollisionComponent>(playerEntity);

				SDL_FRect playerBox{playerPos.x + playerCombat.hitbox.x, playerPos.y + playerCombat.hitbox.y,
				                    playerCombat.hitbox.w, playerCombat.hitbox.h};

				if (SDL_HasRectIntersectionFloat(&enemyBox, &playerBox)) {
					if (registry.has<VelocityComponent>(enemyEntity)) {
						auto &[velocity] = registry.get<VelocityComponent>(enemyEntity);
						velocity.x = 0.0f;
						velocity.y = 0.0f;
					}

					if (registry.has<VelocityComponent>(playerEntity)) {
						auto &[playerVel] = registry.get<VelocityComponent>(playerEntity);
						playerVel.y = pushbackForceY;
					}

				}
			}
		}
	}
};
} // namespace GameEngine::ECS::Systems
