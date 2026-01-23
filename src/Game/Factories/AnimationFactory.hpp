#pragma once
#include "Engine/ECS/Components/Render/AnimationComponent.hpp"
#include "Engine/ECS/Entity.hpp"
#include "Engine/Graphics/TextureManager.hpp"
#include "tileson.hpp"

namespace GameEngine::ECS {
class Registry;
}

class AnimationFactory {
  public:
	AnimationFactory(GameEngine::ECS::Registry &registry, TextureManager &textureManager)
	    : registry(registry), textureManager(textureManager)
	{
	}

	AnimationFactory(const AnimationFactory &) = delete;
	AnimationFactory &operator=(const AnimationFactory &) = delete;
	AnimationFactory(AnimationFactory &&) = delete;
	AnimationFactory &operator=(AnimationFactory &&) = delete;
	~AnimationFactory() = default;

	void setMap(tson::Map &tsonMap) { map = &tsonMap; }
	static AnimationType getAnimationType(const std::string &string);
	void loadAnimation(tson::Tile *const &tile, GameEngine::ECS::Entity entity) const;
	void loadAnimationsFromTileset(tson::Object &object, GameEngine::ECS::Entity entity) const;
	static void loadProjectileAnimation(GameEngine::ECS::Registry &registry, TextureManager &textureManager,
	                                    const std::string &tilesetPath, GameEngine::ECS::Entity entity);
	static void loadAnimationFromStandaloneTileset(GameEngine::ECS::Registry &registry, TextureManager &textureManager,
	                                               tson::Tileset &tileset, const std::string &tilesetPath,
	                                               const GameEngine::ECS::Entity entity);

  private:
	GameEngine::ECS::Registry &registry;
	TextureManager &textureManager;
	tson::Map *map = nullptr;
	const std::filesystem::path basePath = "assets/tiled/tilesets/";
};