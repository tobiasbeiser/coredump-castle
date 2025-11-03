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
#include <Engine/ECS/Components/Gameplay/MeleeComponent.hpp>

struct PlayerFactory {
	static GameEngine::ECS::Entity CreatePlayer(GameEngine::ECS::Registry &registry, TextureManager &texMgr,
	                                            const glm::vec2 spawn, const fs::path &texturePath,
	                                            const int layerIndex)
	{
		auto e = registry.create();
		registry.register_component(e, PlayerTag{});
		registry.register_component(e, PositionComponent{.position = spawn});
		registry.register_component(e, VelocityComponent{.velocity = {0.0f, 0.0f}});
		registry.register_component(e, AccelerationComponent{.acceleration = {0.0f, 0.0f}});
		registry.register_component(e, GravityComponent{});
		registry.register_component(e, GroundedComponent{});
		registry.register_component(e, CollisionComponent{19, 25, 21, 32});

		registry.register_component(e, RenderComponent{.layer = layerIndex, .flipX = false});
		registry.register_component(e, SpriteComponent{.texture = &texMgr.GetTexture(texturePath.string()),
		                                               .TileSize = {56, 56},
		                                               .TileMapPosition = {0, 0}});
		registry.register_component(e, CenterCameraComponent{});

		AnimationComponent playerAnim{};

		playerAnim.addAnimationFromSpritesheet(States::Idle, 6, 0.6f, &texMgr.GetTexture(texturePath.string()),
		                                       {56, 56}, {0, 0});

		playerAnim.addAnimationFromSpritesheet(States::Attacking, 6, 0.6f, &texMgr.GetTexture(texturePath.string()),
		                                       {56, 56}, {0, 56});

		playerAnim.addAnimationFromSpritesheet(States::ConsecutiveAttacking, 2, 0.2f,
		                                       &texMgr.GetTexture(texturePath.string()), {56, 56}, {336, 56});

		playerAnim.addAnimationFromSpritesheet(States::Walking, 8, 0.8f, &texMgr.GetTexture(texturePath.string()),
		                                       {56, 56}, {0, 112});

		playerAnim.addAnimationFromSpritesheet(States::Jumping, 8, 0.4f, &texMgr.GetTexture(texturePath.string()),
		                                       {56, 56}, {0, 168});

		playerAnim.addAnimationFromSpritesheet(States::Falling, 8, 0.4f, &texMgr.GetTexture(texturePath.string()),
		                                       {56, 56}, {0, 224});

		registry.register_component(e, playerAnim);

		registry.register_component(e, StateComponent{States::Idle});
		registry.register_component(e, InputComponent{});
		registry.register_component(e, MovementComponent{});
		registry.register_component(e, JumpComponent{});

		registry.register_component(e, InteractionComponent{19, 25, 21, 32});
		registry.register_component(e, MeleeComponent{});
		return e;
	}
};
