#pragma once
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/JumpComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/AccelerationComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Render/AnimationComponent.hpp"
#include "Engine/ECS/Components/Render/CenterCameraComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Render/TextureComponent.hpp"
#include "Engine/ECS/Components/Render/TileComponent.hpp"
#include "Engine/ECS/Components/Render/CenterCameraComponent.hpp"

#include "Engine/ECS/Components/Physics/GravityComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/Graphics/TextureManager.hpp"

struct PlayerFactory {
	static GameEngine::ECS::Entity CreatePlayer(GameEngine::ECS::Registry &registry, TextureManager &texMgr,
	                                            glm::vec2 spawn, const fs::path texturePath, const int layerIndex)
	{
		SDL_Log("Spawning player");

		auto e = registry.create();
		registry.register_component(e, PositionComponent{.position = spawn});
		registry.register_component(e, VelocityComponent{.velocity = {0.0f, 0.0f}});
		registry.register_component(e, AccelerationComponent{.acceleration = {0.0f, 0.0f}});
		registry.register_component(e, GravityComponent{});
		registry.register_component(e, GroundedComponent{});
		registry.register_component(e, CollisionComponent{19, 25, 21, 32});

		registry.register_component(e, TextureComponent{&texMgr.GetTexture(texturePath.string())});
		registry.register_component(e, RenderComponent{.layer = layerIndex, .flipX = false});
		registry.register_component(e, TileComponent{.TileSize = 56, .TileMapPosition = {0, 0}});
		registry.register_component(e, CenterCameraComponent{});

		AnimationComponent playerAnim{
		    .animations =
		        std::vector<Animation>{
		            Animation(6, 0.6f),        // Idle
		            Animation(6, 0.6f),        // Attack
		            Animation(8, 0.8f),        // Walking
		            Animation(8, 0.4f, false), // Jumping
		            Animation(8, 0.4f, false)  // Falling
		        },
		};
		playerAnim.stateToAnimationIndex = {{States::Idle, 0},
		                                    {States::Attacking, 1},
		                                    {States::Walking, 2},
		                                    {States::Jumping, 3},
		                                    {States::Falling, 4}};

		registry.register_component(e, std::move(playerAnim));
		registry.register_component(e, StateComponent{States::Idle});
		registry.register_component(e, InputComponent{});
		registry.register_component(e, MovementComponent{});
		registry.register_component(e, JumpComponent{});
		SDL_Log("Player spawned");
		return e;
	}
};
