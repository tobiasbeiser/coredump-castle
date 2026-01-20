#include "SpikeSystem.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/PendingDamageComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Hazard/LastGroundedPositionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Hazard/SpikeComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/View.hpp"

#include <SDL3/SDL_log.h>

void GameEngine::ECS::Systems::SpikeSystem::update(Registry &registry)
{
	auto spikeView = registry.view<SpikeComponent, CollisionComponent, PositionComponent>();

	for (auto view = registry.view<LastGroundedPositionComponent, CollisionComponent, PositionComponent>();
	     const auto entity : view) {
		for (const auto spikeEntity : spikeView) {
			if (entity == spikeEntity) {
				continue;
			}

			auto &[collisionBoxDynamic] = registry.get<CollisionComponent>(entity);
			auto &[collisionBoxSpike] = registry.get<CollisionComponent>(spikeEntity);
			const auto &[position, previousPosition] = registry.get<PositionComponent>(entity);
			const auto &[spikePosition, previousSpikePosition] = registry.get<PositionComponent>(spikeEntity);

			auto scaledDynamicBox = SDL_FRect{position.x + collisionBoxDynamic.x, position.y + collisionBoxDynamic.y,
			                                  collisionBoxDynamic.w, collisionBoxDynamic.h};

			auto scaledSpikeBox =
			    SDL_FRect{spikePosition.x + collisionBoxSpike.x, spikePosition.y + collisionBoxSpike.y,
			              collisionBoxSpike.w, collisionBoxSpike.h};

			if (SDL_HasRectIntersectionFloat(&scaledDynamicBox, &scaledSpikeBox)) {
				constexpr int spikeDamage = 10;

				if (!registry.has<PendingDamageComponent>(entity)) {
					registry.registerComponent(entity, PendingDamageComponent{spikeDamage, DamageSource::Hazard});
				} else {
					// override existing damage sources as spikes have priority
					auto &[damage, source] = registry.get<PendingDamageComponent>(entity);
					damage = spikeDamage;
					source = DamageSource::Hazard;
				}
			}
		}
	}
}
