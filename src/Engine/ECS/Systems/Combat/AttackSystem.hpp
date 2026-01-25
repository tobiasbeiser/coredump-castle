#pragma once
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Registry.hpp" // Or wherever 'Entity' is defined
#include <functional>

namespace GameEngine::ECS {
class Registry;
}

namespace GameEngine::ECS::Systems::AttackSystem {

// Now the compiler knows what std::function and Entity are
using AttackCallback = std::function<void(Entity)>;

bool isAttacking(const AttackComponent &attack);

void update(Registry &registry, float deltaTime, const AttackCallback &onAttack);

} // namespace GameEngine::ECS::Systems::AttackSystem