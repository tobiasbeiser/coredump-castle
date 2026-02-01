#include "ReviveSystem.hpp"

#include "Engine/ECS/Components/Gameplay/Combat/HealthComponent.hpp"
#include "Engine/ECS/Components/Tags/PendingReviveTag.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Game/States/FadeTransitionState.hpp"

void GameEngine::ECS::Systems::ReviveSystem::update(Registry &registry, GameEngine &game, const float deltaTime)
{
	for (auto view = registry.view<PendingReviveTag>(); const auto entity : view) {
		if (view.size() > 1) {
			throw std::runtime_error("There cannot be more than on entity with a PendingReviveTag");
		}
		auto reviveLogic = [&registry, entity]() {
			auto lambdaView = registry.view<HealthComponent, SpriteComponent, CollisionComponent>();
			auto &healthComp = lambdaView.get<HealthComponent>(entity);
			auto &spriteComp = lambdaView.get<SpriteComponent>(entity);
			auto &[collisionBox] = lambdaView.get<CollisionComponent>(entity);


			healthComp.health = healthComp.maxHealth;
			healthComp.invulnerabilityTimer.reset();
			spriteComp.colorMod = {255, 255, 255, 255};
			registry.registerComponent(entity, VelocityComponent{});
			registry.registerComponent(entity, InputComponent{});
			registry.registerComponent(entity, HurtBoxComponent{collisionBox});
		};
		game.pushState(std::make_unique<FadeTransitionState>(reviveLogic));
		game.getSaveSystem().loadSavedGame(game.getRegistry());
	}
}
