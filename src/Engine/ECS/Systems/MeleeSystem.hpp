#pragma once
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MeleeComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include <Engine/ECS/Components/Core/PositionComponent.hpp>
#include <Engine/ECS/Components/Core/StateComponent.hpp>

namespace GameEngine::ECS::Systems {
struct MeleeSystem {
	static void Update(Registry &registry, float deltaTime)
	{
		auto view = registry.view<MeleeComponent, InputComponent>();
		for (auto entity : view) {
			auto &melee = registry.get<MeleeComponent>(entity);
			auto &input = registry.get<InputComponent>(entity);
			auto &pos = registry.get<PositionComponent>(entity);

			if (input.meleePressed && !melee.isAttacking) {
				melee.isAttacking = true;
				melee.attackTimer = 0.0f;
				if (registry.has<StateComponent>(entity)) {
					registry.get<StateComponent>(entity).state = States::Attacking;
				}
			}

			if (melee.isAttacking) {
				melee.attackTimer += deltaTime;
				if (registry.has<StateComponent>(entity)) {
					registry.get<StateComponent>(entity).state = States::Attacking;
				}
				if (melee.attackTimer >= melee.attackDuration) {
					melee.isAttacking = false;
					melee.attackTimer = 0.0f;
					if (registry.has<StateComponent>(entity)) {
						registry.get<StateComponent>(entity).state = States::Idle;
					}
				}
			}
		}
	}
};

} // namespace GameEngine::ECS::Systems