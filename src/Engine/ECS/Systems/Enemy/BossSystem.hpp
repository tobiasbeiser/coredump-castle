#pragma once

#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/System.hpp"

namespace GameEngine::ECS::Systems {
class BossSystem : public System {
  public:
	BossSystem(GameEngine::ECS::Registry &registry) : System(registry) {}

	void update(float deltaTime) override;
};
} // namespace GameEngine::ECS::Systems