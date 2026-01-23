#pragma once
#include "AnimationFactory.hpp"
#include "Engine/ECS/Components/Gameplay/Hazard/LastGroundedPositionComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/Graphics/TextureManager.hpp"

class PlayerFactory {
  public:
	PlayerFactory(GameEngine::ECS::Registry &registry, TextureManager &tex_mgr, AnimationFactory &animation_factory);

	PlayerFactory(PlayerFactory &&) = delete;
	PlayerFactory(const PlayerFactory &) = delete;
	PlayerFactory &operator=(const PlayerFactory &) = delete;
	PlayerFactory &operator=(PlayerFactory &&) = delete;
	~PlayerFactory() = default;
	GameEngine::ECS::Entity createPlayer(tson::Object &player, const glm::vec2 &levelPos) const;

  private:
	GameEngine::ECS::Registry &registry;
	TextureManager &texMgr;
	AnimationFactory &animationFactory;
};
