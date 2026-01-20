#include "SpikeSystem.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/PendingDamageComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Hazard/LastGroundedPositionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Hazard/SpikeComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/View.hpp"

void GameEngine::ECS::Systems::SpikeSystem::update(Registry &registry)
{
	auto spikeView = registry.view<SpikeComponent, CollisionComponent>();

	for (auto view = registry.view<LastGroundedPositionComponent, CollisionComponent>(); const auto entity : view) {

		for (const auto spikeEntity : spikeView) {
			auto &[collisionBoxDynamic] = registry.get<CollisionComponent>(entity);
			auto &[collisionBoxSpike] = registry.get<CollisionComponent>(spikeEntity);

			if (SDL_HasRectIntersectionFloat(&collisionBoxDynamic, &collisionBoxSpike)) {
				constexpr int spikeDamage = 10;

				if (!registry.has<PendingDamageComponent>(entity)) {
					registry.registerComponent(entity, PendingDamageComponent{spikeDamage});
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
