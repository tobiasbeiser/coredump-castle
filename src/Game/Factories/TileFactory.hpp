#pragma once
#include "AnimationFactory.hpp"
#include "Engine/ECS/Entity.hpp"
#include "Engine/ECS/Registry.hpp"
#include "tileson.hpp"
#include <glm/vec2.hpp>

class TileFactory {
  public:
	TileFactory(GameEngine::ECS::Registry &registry, TextureManager &textureManager,
	            AnimationFactory &animationFactory);

	TileFactory(TileFactory &&) = delete;
	TileFactory(const TileFactory &) = delete;
	TileFactory &operator=(const TileFactory &) = delete;
	TileFactory &operator=(TileFactory &&) = delete;
	~TileFactory() = default;

	GameEngine::ECS::Entity createTile(tson::Tile &tile, const glm::vec2 &worldPosition, int layerIndex) const;

	std::optional<GameEngine::ECS::Entity> createFromObject(const tson::Object &object, const glm::vec2 &levelPos,
	                                                        int layerIndex) const;

	std::optional<tson::Tile> getTileFromObject(const tson::Object &object) const;
	void setMap(tson::Map &tsonMap);

  private:
	GameEngine::ECS::Registry &registry;
	TextureManager &textureManager;
	AnimationFactory &animationFactory;
	tson::Map *map = nullptr;

	void addProperties(GameEngine::ECS::Entity entity, tson::Tile &tile) const;
	void addCollision(GameEngine::ECS::Entity entity, tson::Tile &tile) const;
	void addHazard(GameEngine::ECS::Entity entity, tson::Tile &tile) const;
};
