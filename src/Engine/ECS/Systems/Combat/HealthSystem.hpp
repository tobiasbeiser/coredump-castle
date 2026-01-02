#pragma once
#include "Engine/ECS/Components/Gameplay/Combat/HealthComponent.hpp"

namespace GameEngine::ECS {
class Registry;
}

namespace GameEngine::ECS::Systems::HealthSystem {

void update(Registry &registry, float deltaTime);

bool isHurting(const HealthComponent &health);
bool isDead(const HealthComponent &health);
bool isInvulnerable(const HealthComponent &health);

} // namespace GameEngine::ECS::Systems::HealthSystem