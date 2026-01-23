#include "SpikeSystem.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/PendingDamageComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Hazard/LastGroundedPositionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Hazard/PendingRespawnComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Hazard/SpikeComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/View.hpp"

void GameEngine::ECS::Systems::SpikeSystem::update(Registry &registry)
{
	auto spikeView = registry.view<SpikeComponent, CollisionComponent, PositionComponent>();

	for (auto view = registry.view<LastGroundedPositionComponent, CollisionComponent, PositionComponent>()
	                     .exclude<PendingRespawnComponent>();
	     const auto entity : view) {
		for (const auto spikeEntity : spikeView) {
			if (entity == spikeEntity) {
				continue;
			}

			auto &[collisionBoxDynamic] = view.get<CollisionComponent>(entity);
			auto &[collisionBoxSpike] = view.get<CollisionComponent>(spikeEntity);
			const auto &[position, previousPosition] = view.get<PositionComponent>(entity);
			const auto &[spikePosition, previousSpikePosition] = view.get<PositionComponent>(spikeEntity);

			auto scaledDynamicBox = SDL_FRect{position.x + collisionBoxDynamic.x, position.y + collisionBoxDynamic.y,
			                                  collisionBoxDynamic.w, collisionBoxDynamic.h};

			auto scaledSpikeBox =
			    SDL_FRect{spikePosition.x + collisionBoxSpike.x, spikePosition.y + collisionBoxSpike.y,
			              collisionBoxSpike.w, collisionBoxSpike.h};

			if (SDL_HasRectIntersectionFloat(&scaledDynamicBox, &scaledSpikeBox)) {
				constexpr int spikeDamage = 10;

				const auto pendingDamage = registry.tryGet<PendingDamageComponent>(entity);

				if (!pendingDamage) {
					registry.registerComponent(entity, PendingDamageComponent{spikeDamage, DamageSource::Hazard});
				} else {
					// override existing damage sources as spikes have priority
					auto &[damage, source] = pendingDamage->get();
					damage = spikeDamage;
					source = DamageSource::Hazard;
				}
			}
		}
	}
}
