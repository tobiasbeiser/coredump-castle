#pragma once
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/PushbackComponent.hpp"
#include "Engine/ECS/Components/Physics/AccelerationComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Components/Tags/SpikeTag.hpp"
#include "Engine/ECS/Registry.hpp"

#include <Engine/ECS/Components/Render/BlinkComponent.hpp>
#include <SDL3/SDL_rect.h>
#include <cmath>

namespace GameEngine::ECS::Systems {
struct PushbackSystem {
	static void update(Registry &registry, float deltaTime)
	{

		pushBackBySpikes(registry);

		auto enemyView =
		    registry.view<EnemyTag, PushbackComponent, HurtBoxComponent, PositionComponent, VelocityComponent>();

		auto playerView = registry.view<PlayerTag, HurtBoxComponent, PositionComponent>();

		for (const auto enemyEntity : enemyView) {
			const auto &[enemyPos, _] = registry.get<PositionComponent>(enemyEntity);
			const auto [enemyHitbox] = registry.get<HurtBoxComponent>(enemyEntity);
			const auto &[pushbackForceY] = registry.get<PushbackComponent>(enemyEntity);

			SDL_FRect enemyBox{enemyPos.x + enemyHitbox.x, enemyPos.y + enemyHitbox.y, enemyHitbox.w, enemyHitbox.h};

			for (const auto playerEntity : playerView) {
				auto &[playerPos, _] = registry.get<PositionComponent>(playerEntity);
				const auto [playerHitbox] = registry.get<HurtBoxComponent>(playerEntity);

				SDL_FRect playerBox{playerPos.x + playerHitbox.x, playerPos.y + playerHitbox.y, playerHitbox.w,
				                    playerHitbox.h};

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

					auto &blink = registry.get<BlinkComponent>(playerEntity);
					blink.timer.reset();
				}
			}
		}
	}

	static void pushBackBySpikes(Registry &registry)
	{
		auto spikeView = registry.view<SpikeTag, HurtBoxComponent, PositionComponent>();

		auto playerView = registry.view<PlayerTag, HurtBoxComponent, PositionComponent>();

		for (auto spike : spikeView) {
			const auto &[spikePos, _] = registry.get<PositionComponent>(spike);
			const auto &[spikeHitbox] = registry.get<HurtBoxComponent>(spike);

			SDL_FRect sBox{spikePos.x + spikeHitbox.x, spikePos.y + spikeHitbox.y, spikeHitbox.w, spikeHitbox.h};

			for (const auto player : playerView) {
				const auto &[playerPos, _] = registry.get<PositionComponent>(player);
				const auto [playerHitbox] = registry.get<HurtBoxComponent>(player);
				const auto &[pushbackForceY] = registry.get<PushbackComponent>(player);

				SDL_FRect pBox{playerPos.x + playerHitbox.x, playerPos.y + playerHitbox.y, playerHitbox.w,
				               playerHitbox.h};

				if (SDL_HasRectIntersectionFloat(&sBox, &pBox)) {
					if (registry.has<VelocityComponent>(player)) {
						auto &[playerVel] = registry.get<VelocityComponent>(player);
						playerVel.y = pushbackForceY;
					}

					auto &blink = registry.get<BlinkComponent>(player);
					blink.timer.reset();
				}
			}
		}
	}
};
} // namespace GameEngine::ECS::Systems
