
#include "HudSystem.hpp"

#include "Engine/ECS/Components/Gameplay/BossComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HealthComponent.hpp"
#include "Engine/ECS/Components/Render/HealthBarComponent.hpp"
#include "Engine/ECS/Components/Render/HudOwnerComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/Components/Tags/InactiveTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"

void GameEngine::ECS::Systems::HudSystem::update(Registry &registry)
{

	PROFILE_SCOPE("HudSystem");
	for (auto view = registry.view<HudOwnerComponent>(); const auto entity : view) {
		auto &[type, trackedEntity] = view.get<HudOwnerComponent>(entity);

		if (trackedEntity == invalidEntity) {
			if (type == HudOwnerType::Player) {
				if (auto players = registry.view<PlayerTag>(); players.size() == 1U) {
					trackedEntity = players.at(0U);
				}
			} else if (type == HudOwnerType::Boss) {
				// Find a living boss
				if (auto bossView = registry.view<BossComponent>().exclude<InactiveTag>(); bossView.size() == 1U) {
					trackedEntity = bossView.at(0U);
				}
			}
		}

		if (trackedEntity == invalidEntity) {
			if (!registry.has<InactiveTag>(entity)) {
				registry.registerComponent(entity, InactiveTag{});
			}
			continue;
		}

		if (!registry.exists(trackedEntity) || registry.has<InactiveTag>(trackedEntity)) {
			// disable boss hud
			if (!registry.has<InactiveTag>(entity)) {
				registry.registerComponent(entity, InactiveTag{});
			}
			trackedEntity = invalidEntity;
			continue;
		}

		if (registry.has<InactiveTag>(entity))
			registry.remove<InactiveTag>(entity);

		if (const auto barComp = registry.tryGet<HealthBarComponent>(entity)) {
			float currentPercentage = 1.f;
			const auto &health = registry.tryGet<HealthComponent>(trackedEntity);
			if (health.has_value()) {
				const float maxHealth = (health->get().maxHealth > 0.0f) ? health->get().maxHealth : 1.0f;
				currentPercentage = std::clamp(health->get().health / maxHealth, 0.0f, 1.0f);
			} else {
				currentPercentage = 0.f;
			}

			auto &sprite = registry.get<SpriteComponent>(entity);
			sprite.tileSize.x = barComp->get().originalWidth * currentPercentage;
		}
	}
}
