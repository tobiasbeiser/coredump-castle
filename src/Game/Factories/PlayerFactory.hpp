#pragma once
#include "AnimationFactory.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InteractionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InventoryComponent.hpp"
#include "Engine/ECS/Components/Gameplay/JumpComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/AccelerationComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Physics/GravityComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Render/AnimationComponent.hpp"
#include "Engine/ECS/Components/Render/CenterCameraComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/Graphics/TextureManager.hpp"
#include <Engine/ECS/Components/Gameplay/MeleeComponent.hpp>

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
		registry.registerComponent(e, CenterCameraComponent{});
		registry.registerComponent(e, InventoryComponent{});

		registry.registerComponent(e, StateComponent{});

		//constexpr auto meleeComp = MeleeComponent{};
		//registry.register_component(e, meleeComp);


		//playerAnim.addAnimationFromSpritesheet(States::Idle, 6, 0.6f, &texMgr.GetTexture(texturePath.string()),
		/*									   {56, 56}, {0, 0});

		playerAnim.addAnimationFromSpritesheet(States::Attacking, 6, meleeComp.attackDuration, &texMgr.GetTexture(texturePath.string()),
											   {56, 56}, {0, 56});

		playerAnim.addAnimationFromSpritesheet(States::ConsecutiveAttacking, 2, 0.2f,
											   &texMgr.GetTexture(texturePath.string()), {56, 56}, {336, 56});

*/
		if (const auto prop = player.getProp("animations"); prop != nullptr && prop->getType() == tson::Type::File) {

			const std::filesystem::path basePath = "assets/tiled/tilesets/";
			const auto tilesetPath = basePath / prop->getValue<std::filesystem::path>();
			tson::Tileset tileset;

			tson::Json11 json;
			json.parse(tilesetPath);
			tileset.parse(json, &map); // map is required to read project enums

			const AnimationFactory animFactory(registry, texMgr);

			for (auto tile : tileset.getTiles()) {
				if (auto anim = tile.getAnimation(); anim.any()) {
					animFactory.loadAnimation(&tile, e);
				}
			}
		}

		registry.registerComponent(e, InputComponent{});
		registry.registerComponent(e, MovementComponent{});
		registry.registerComponent(e, JumpComponent{});

		registry.registerComponent(e, InteractionComponent{19, 25, 21, 32});
		return e;
	}
};
