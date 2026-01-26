#include "PlayerFactory.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HealthComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InteractionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InventoryComponent.hpp"
#include "Engine/ECS/Components/Gameplay/JumpComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MapToggleComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Gameplay/ProgressComponent.hpp"
#include "Engine/ECS/Components/Physics/AccelerationComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Physics/GravityComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Render/BlinkComponent.hpp"
#include "Engine/ECS/Components/Render/CenterCameraComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"

#include "Engine/ECS/Components/Core/SoundComponent.hpp"

PlayerFactory::PlayerFactory(GameEngine::ECS::Registry &registry, TextureManager &tex_mgr,
                             AnimationFactory &animation_factory, AudioManager &audioManager)
    : registry(registry), texMgr(tex_mgr), animationFactory(animation_factory), audioManager(audioManager)
{
}
GameEngine::ECS::Entity PlayerFactory::createPlayer(tson::Object &player, const glm::vec2 &levelPos) const
{
	{
		const auto e = registry.create();
		registry.registerComponent(e, PlayerTag{});

		const tson::Vector2 pos = player.getPosition();
		const glm::vec2 spawn = glm::vec2(pos.x, pos.y) + levelPos;

		constexpr SDL_FRect collisionBox = {18, 25, 15, 32};

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
		registry.registerComponent(e, ProgressComponent{});

		registry.registerComponent(e, StateComponent{});

		using namespace std::chrono_literals;

		animationFactory.loadAnimationsFromTileset(player, e);
		std::chrono::duration<float> attackDuration = 1s;
		if (const auto animComp = registry.tryGet<AnimationComponent>(e)) {
			if (animComp->get().hasAnimation(AnimationType::Attacking)) {
				attackDuration = animComp->get().getAnimationDuration(AnimationType::Attacking);
			}
		}

		registry.registerComponent(
		    e, AttackComponent{.attackTimer = Timer{attackDuration}, .attackCoolDownTimer = Timer{0.41s}});

		registry.registerComponent(e, InputComponent{});
		registry.registerComponent(e, MovementComponent{});
		registry.registerComponent(e, MapToggleComponent{});

		registry.registerComponent(e, JumpComponent{.maxJumps = 1, .coyoteTimer = Timer{0.08s}});
		registry.registerComponent(e, BlinkComponent{.color = {50, 50, 50, 150}});

		registry.registerComponent(e, InteractionComponent{19, 25, 21, 32});
		registry.registerComponent(e, HurtBoxComponent{collisionBox});
		constexpr int weaponHeight = 45;
		constexpr int weaponOffsetY = -(weaponHeight - collisionBox.h) / 2;
		registry.registerComponent(e, HitBoxComponent{.weaponHitBox = {0, weaponOffsetY, 35, weaponHeight}});
		registry.registerComponent(e, HealthComponent{60, 60, 1.75s});

		audioManager.loadSoundGroup("player_jump", "assets/audio/sound/player/jump");
		audioManager.loadSoundGroup("player_land", "assets/audio/sound/player/landing");
		audioManager.loadSoundGroup("player_atk", "assets/audio/sound/player/attack");
		audioManager.loadSoundGroup("player_walk", "assets/audio/sound/player/walking");
		audioManager.loadSoundGroup("player_hurt", "assets/audio/sound/player/hurt");
		audioManager.loadSoundGroup("player_die", "assets/audio/sound/player/death");

		SoundComponent soundComp;
		soundComp.sounds[AnimationType::JumpStart] = "player_jump";
		soundComp.sounds[AnimationType::Landing] = "player_land";
		soundComp.sounds[AnimationType::Attacking] = "player_atk";
		soundComp.sounds[AnimationType::Walking] = "player_walk";
		soundComp.sounds[AnimationType::Hurting] = "player_hurt";
		soundComp.sounds[AnimationType::Dead] = "player_die";
		soundComp.stepTimer = Timer{0.35s};

		registry.registerComponent<SoundComponent>(e, soundComp);

		return e;
	}
}