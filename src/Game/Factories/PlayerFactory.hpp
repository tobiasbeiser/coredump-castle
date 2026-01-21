#pragma once
#include "AnimationFactory.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HealthComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Hazard/LastGroundedPositionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InteractionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InventoryComponent.hpp"
#include "Engine/ECS/Components/Gameplay/JumpComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/AccelerationComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Physics/GravityComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Render/BlinkComponent.hpp"
#include "Engine/ECS/Components/Render/CenterCameraComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/Graphics/TextureManager.hpp"

class PlayerFactory {
  public:
	PlayerFactory(GameEngine::ECS::Registry &registry, TextureManager &tex_mgr, AnimationFactory &animation_factory)
	    : registry(registry), texMgr(tex_mgr), animationFactory(animation_factory)
	{
	}

	PlayerFactory(PlayerFactory &&) = delete;
	PlayerFactory(const PlayerFactory &) = delete;
	PlayerFactory &operator=(const PlayerFactory &) = delete;
	PlayerFactory &operator=(PlayerFactory &&) = delete;
	~PlayerFactory() = default;
	GameEngine::ECS::Entity createPlayer(tson::Object &player, const glm::vec2 &levelPos) const
	{
		const auto e = registry.create();
		registry.registerComponent(e, PlayerTag{});

		const tson::Vector2 pos = player.getPosition();
		const glm::vec2 spawn = glm::vec2(pos.x, pos.y) + levelPos;

		SDL_FRect collisionBox = {18, 25, 15, 32};

		registry.registerComponent(e, PositionComponent{.position = spawn});
		registry.registerComponent(e, LastGroundedPositionComponent{.position = spawn});
		registry.registerComponent(e, VelocityComponent{.velocity = {0.0f, 0.0f}});
		registry.registerComponent(e, AccelerationComponent{.acceleration = {0.0f, 0.0f}});
		registry.registerComponent(e, GravityComponent{});
		registry.registerComponent(e, GroundedComponent{});
		registry.registerComponent(e, CollisionComponent{collisionBox});
		registry.registerComponent(e, RenderComponent{.layer = 100, .flipX = false});
		registry.registerComponent(e, TransformComponent{});
		registry.registerComponent(e, CenterCameraComponent{});
		registry.registerComponent(e, InventoryComponent{});

		registry.registerComponent(e, StateComponent{});

		using namespace std::chrono_literals;

		animationFactory.loadAnimationsFromTileset(player, e);
		std::chrono::duration<float> attackDuration = 1s;
		if (registry.has<AnimationComponent>(e)) {
			auto &animComp = registry.get<AnimationComponent>(e);
			if (animComp.hasAnimation(AnimationType::Attacking)) {
				attackDuration = animComp.getAnimationDuration(AnimationType::Attacking);
				SDL_Log("Attack duration: %f", attackDuration.count());
			}
		}

		registry.registerComponent(
		    e, AttackComponent{.attackTimer = Timer{attackDuration}, .attackCoolDownTimer = Timer{0.2s}});

		registry.registerComponent(e, InputComponent{});
		registry.registerComponent(e, MovementComponent{});
		registry.registerComponent(e, JumpComponent{.maxJumps = 5, .coyoteTimer = Timer{0.08s}});
		registry.registerComponent(e, BlinkComponent{});

		registry.registerComponent(e, InteractionComponent{19, 25, 21, 32});
		registry.registerComponent(e, HurtBoxComponent{19, 25, 21, 32});
		registry.registerComponent(e, HitBoxComponent{.weaponHitBox = {0, -5, 35, 45}});
		registry.registerComponent(e, HealthComponent{100, 300, 0.6s});

		return e;
	}

  private:
	GameEngine::ECS::Registry &registry;
	TextureManager &texMgr;
	AnimationFactory &animationFactory;
};
