#pragma once
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/PushbackComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Components/Tags/SpikeTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/Utils/PhysicsUtils.hpp"
#include <Engine/ECS/Components/Render/BlinkComponent.hpp>
#include <SDL3/SDL_rect.h>

namespace GameEngine::ECS::Systems {
struct PushbackSystem {
	static void update(Registry &registry)
	{
		handleSpikeCollision(registry);
		handleEnemyCollision(registry);
	}

	private:
	static void handleEnemyCollision(Registry &registry)
	{
		auto enemyView = registry.view<EnemyTag, PushbackComponent, HitBoxComponent, PositionComponent, VelocityComponent>();
		auto playerView = registry.view<PlayerTag, HitBoxComponent, PositionComponent, VelocityComponent>();

		for (const auto enemyEntity : enemyView) {
			const auto &[enemyPos, _] = registry.get<PositionComponent>(enemyEntity);
			const auto &[enemyHitbox] = registry.get<HitBoxComponent>(enemyEntity);
			const auto &[pushbackForceY] = registry.get<PushbackComponent>(enemyEntity);

			SDL_FRect enemyBounds = Utils::getBoundingBox(enemyPos, enemyHitbox);

			for (const auto playerEntity : playerView) {
				auto &[playerPos, _] = registry.get<PositionComponent>(playerEntity);
				const auto &[playerHitbox] = registry.get<HitBoxComponent>(playerEntity);

				SDL_FRect playerBounds = Utils::getBoundingBox(playerPos, playerHitbox);

				if (SDL_HasRectIntersectionFloat(&enemyBounds, &playerBounds)) {
					auto &[velocity] = registry.get<VelocityComponent>(enemyEntity);
					velocity.x = 0.0f;
					velocity.y = 0.0f;

					applyPushBackToEntity(registry, playerEntity, pushbackForceY);
				}
			}
		}
	}

	static void applyPushBackToEntity(Registry & registry, const unsigned entity, const float pushBackForceY)
	{
		auto &[playerVelocity] = registry.get<VelocityComponent>(entity);
		playerVelocity.y = pushBackForceY;

		auto &blink = registry.get<BlinkComponent>(entity);
		blink.timer.reset();
	}

	static void handleSpikeCollision(Registry &registry)
	{
		auto spikeView = registry.view<SpikeTag, HitBoxComponent, PositionComponent>();

		auto playerView = registry.view<PlayerTag, HitBoxComponent, PositionComponent, VelocityComponent>();

		for (const auto spike : spikeView) {
			const auto &[spikePos, _] = registry.get<PositionComponent>(spike);
			const auto &[spikeHitbox] = registry.get<HitBoxComponent>(spike);

			SDL_FRect spikesBounds = Utils::getBoundingBox(spikePos, spikeHitbox);

			for (const auto player : playerView) {
				const auto &[playerPos, _] = registry.get<PositionComponent>(player);
				const auto &[playerHitbox] = registry.get<HitBoxComponent>(player);
				const auto &[pushbackForceY] = registry.get<PushbackComponent>(player);

				SDL_FRect playerBounds = Utils::getBoundingBox(playerPos, playerHitbox);

				if (SDL_HasRectIntersectionFloat(&spikesBounds, &playerBounds)) {
					applyPushBackToEntity(registry, player, pushbackForceY);
				}
			}
		}
	}
};
} // namespace GameEngine::ECS::Systems
