#pragma once
#include "Engine/Debug/DebugSystem.hpp"
#include "Engine/Debug/ScopedProfiler.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/JumpComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"

namespace GameEngine::ECS::Systems {
struct JumpSystem {
	static void update(Registry &registry, const float deltaTime)
	{
		PROFILE_SCOPE("JumpSystem");
		for (auto view = registry.view<JumpComponent, GroundedComponent, VelocityComponent, InputComponent>();
		     const auto entity : view) {

			auto &[jumpStrength, jumpsLeft, maxJumps, coyoteTimer] = view.get<JumpComponent>(entity);
			const auto &[grounded] = view.get<GroundedComponent>(entity);
			auto &[velocity] = view.get<VelocityComponent>(entity);
			const auto &input = view.get<InputComponent>(entity);

			if (registry.has<PlayerTag>(entity) && DebugSystem::instance().getCheats().unlimitedJumps) {
				jumpsLeft = maxJumps;
			}

			coyoteTimer.step(std::chrono::duration<float>(deltaTime), false);
			if (grounded) {
				coyoteTimer.reset();
			}
			if (!coyoteTimer.isTimeout()) {
				jumpsLeft = maxJumps;
			} else if (jumpsLeft == maxJumps) {
				jumpsLeft--; // leave ground without jumping
			}

			if (jumpsLeft && input.jumpPressed) {
				const float multiplier = !coyoteTimer.isTimeout() ? 1.0f : 0.7f;
				velocity.y = -jumpStrength * multiplier;
				jumpsLeft--;
			}
		}
	}
};

} // namespace GameEngine::ECS::Systems