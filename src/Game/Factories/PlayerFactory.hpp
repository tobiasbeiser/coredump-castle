#pragma once
#include "AnimationFactory.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InteractionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InventoryComponent.hpp"
#include "Engine/ECS/Components/Gameplay/JumpComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/AccelerationComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Physics/GravityComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Render/CenterCameraComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/Graphics/TextureManager.hpp"

class PlayerFactory {
  public:
	static GameEngine::ECS::Entity createPlayer(GameEngine::ECS::Registry &registry, TextureManager &texMgr,
	                                            tson::Object &player, const glm::vec2 &levelPos, tson::Map &map)
	{
		const auto e = registry.create();
		registry.registerComponent(e, PlayerTag{});

		const tson::Vector2 pos = player.getPosition();
		const glm::vec2 spawn = glm::vec2(pos.x, pos.y) + levelPos;

		registry.registerComponent(e, PositionComponent{.position = spawn});
		registry.registerComponent(e, VelocityComponent{.velocity = {0.0f, 0.0f}});
		registry.registerComponent(e, AccelerationComponent{.acceleration = {0.0f, 0.0f}});
		registry.registerComponent(e, GravityComponent{});
		registry.registerComponent(e, GroundedComponent{});
		registry.registerComponent(e, CollisionComponent{19, 25, 21, 32});
		registry.registerComponent(e, RenderComponent{.layer = 100, .flipX = false});
		registry.registerComponent(e, TransformComponent{}); // Default scale of 1.0
		registry.registerComponent(e, CenterCameraComponent{});
		registry.registerComponent(e, InventoryComponent{});

		registry.registerComponent(e, StateComponent{});

		using namespace std::chrono_literals;
		registry.registerComponent(e, AttackComponent{.attackTimer = Timer{0.6s}, .attackCoolDownTimer = Timer{0.2s}});

		AnimationFactory::loadAnimationsFromTileset(registry, texMgr, map, player, e);

		registry.registerComponent(e, InputComponent{});
		registry.registerComponent(e, MovementComponent{});
		registry.registerComponent(e, JumpComponent{.maxJumps = 2, .coyoteTimer = Timer{0.08s}});

		registry.registerComponent(e, InteractionComponent{19, 25, 21, 32});
		return e;
	}
};
