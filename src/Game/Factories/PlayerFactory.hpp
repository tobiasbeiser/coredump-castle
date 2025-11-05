#pragma once
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InteractionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/JumpComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/AccelerationComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Render/AnimationComponent.hpp"
#include "Engine/ECS/Components/Render/CenterCameraComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"

#include "Engine/ECS/Components/Physics/GravityComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/Graphics/TextureManager.hpp"

struct PlayerFactory {
	static GameEngine::ECS::Entity createPlayer(GameEngine::ECS::Registry &registry, TextureManager &texMgr,
	                                            const glm::vec2 spawn, const fs::path &texturePath,
	                                            const int layerIndex)
	{
		auto e = registry.create();
		registry.registerComponent(e, PlayerTag{});
		registry.registerComponent(e, PositionComponent{.position = spawn});
		registry.registerComponent(e, VelocityComponent{.velocity = {0.0f, 0.0f}});
		registry.registerComponent(e, AccelerationComponent{.acceleration = {0.0f, 0.0f}});
		registry.registerComponent(e, GravityComponent{});
		registry.registerComponent(e, GroundedComponent{});
		registry.registerComponent(e, CollisionComponent{19, 25, 21, 32});

		registry.registerComponent(e, RenderComponent{.layer = layerIndex, .flipX = false});
		registry.registerComponent(e, SpriteComponent{.texture = &texMgr.GetTexture(texturePath.string()),
		                                               .tileSize = {56, 56},
		                                               .tileMapPosition = {0, 0}});
		registry.registerComponent(e, CenterCameraComponent{});

		AnimationComponent playerAnim{};

		playerAnim.addAnimationFromSpritesheet(States::Idle, 6, 0.6f, &texMgr.GetTexture(texturePath.string()),
		                                       {56, 56}, {0, 0});

		playerAnim.addAnimationFromSpritesheet(States::Attacking, 6, 0.6f, &texMgr.GetTexture(texturePath.string()),
		                                       {56, 56}, {0, 56});

		playerAnim.addAnimationFromSpritesheet(States::Walking, 8, 0.8f, &texMgr.GetTexture(texturePath.string()),
		                                       {56, 56}, {0, 112});

		playerAnim.addAnimationFromSpritesheet(States::Jumping, 8, 0.4f, &texMgr.GetTexture(texturePath.string()),
		                                       {56, 56}, {0, 168});

		playerAnim.addAnimationFromSpritesheet(States::Falling, 8, 0.4f, &texMgr.GetTexture(texturePath.string()),
		                                       {56, 56}, {0, 224});

		registry.registerComponent(e, playerAnim);

		registry.registerComponent(e, StateComponent{States::Idle});
		registry.registerComponent(e, InputComponent{});
		registry.registerComponent(e, MovementComponent{});
		registry.registerComponent(e, JumpComponent{});

		registry.registerComponent(e, InteractionComponent{19, 25, 21, 32});
		return e;
	}
};
