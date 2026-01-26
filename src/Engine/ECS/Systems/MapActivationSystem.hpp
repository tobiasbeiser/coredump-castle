#pragma once
#include "CollisionSystem.hpp"
#include "Engine/Debug/ScopedProfiler.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/Components/Tags/InactiveTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/Utils/ReferenceFrameUtils.hpp"
#include "PhysicsSystem.hpp"

namespace GameEngine::ECS::Systems {
class MapActivationSystem {
  public:
	void update(Registry &registry)
	{
		PROFILE_SCOPE("MapActivationSystem");

		if (const auto referenceFrame = Utils::ReferenceFrameUtils::getReferenceFrame(registry);
		    referenceFrame != previousReferenceFrame) {

			const bool transitionFromBottomToTop =
			    referenceFrame.y + referenceFrame.h < previousReferenceFrame.y + previousReferenceFrame.h;

			previousReferenceFrame = referenceFrame;

			// Activate/deactivate entities if they are inside/outside the current level
			for (auto view = registry.view<PositionComponent>().exclude<PlayerTag>(); const auto entity : view) {
				if (auto &[position, previousPosition] = view.get<PositionComponent>(entity);
				    Utils::ReferenceFrameUtils::isInFrame(position, referenceFrame)) { // entity is in referenceFrame

					if (registry.has<InactiveTag>(entity)) {
						registry.remove<InactiveTag>(entity);
					}
				} else {
					registry.registerComponent(entity, InactiveTag{});
				}
			}

			// Set player layer correctly
			for (auto playerView = registry.view<PlayerTag, RenderComponent, VelocityComponent>();
			     const auto playerEntity : playerView) {
				auto &[layer, flipX] = playerView.get<RenderComponent>(playerEntity);
				layer = referenceFrame.playerLayer;
				if (transitionFromBottomToTop) {
					if (auto &[velocity] = playerView.get<VelocityComponent>(playerEntity); velocity.y < 0) {
						velocity.y = -400;
					}
				}
			}
			CollisionSystem::buildStaticColliderGrid(registry);
		}
	};

  private:
	Utils::ReferenceFrameUtils::ReferenceFrame previousReferenceFrame{};
};
} // namespace GameEngine::ECS::Systems