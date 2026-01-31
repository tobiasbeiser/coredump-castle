#pragma once
#include "EnemyFactory.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/Graphics/TextureManager.hpp"
#include "Engine/Input/InputManager.hpp"
#include "Game/Factories/AnimationFactory.hpp"
#include "Parsers/SavePointParser.hpp"
#include "PlayerFactory.hpp"
#include "TileFactory.hpp"

#include <glm/glm.hpp>
#include <tileson.hpp>

class LevelFactory {
  public:
	LevelFactory(GameEngine::ECS::Registry &registry, TextureManager &textureManager, InputManager &inputManager,
	             AudioManager &audioManager, std::vector<EnemySpawner> &spawners);

	LevelFactory(LevelFactory &&) = delete;
	LevelFactory &operator=(LevelFactory &&) = delete;
	LevelFactory(const LevelFactory &) = delete;
	LevelFactory &operator=(const LevelFactory &) = delete;
	~LevelFactory() = default;

	GameEngine::ECS::Entity createLevel(tson::Map &map, const glm::vec2 &pos);

  private:
	void loadTiles(tson::Layer &layer, int layerIndex, const glm::vec2 &levelPos) const;
	void loadBackground(const tson::Layer &layer, int layerIndex, const glm::vec2 &levelPos) const;
	void createBackground(const glm::vec2 &levelPos, const fs::path &filename, int layer, float scrollSpeed = 0,
	                      float yOffset = 0, bool repeatX = false) const;
	void loadLayer(tson::Layer &layer, int &layerIndex, const glm::vec2 &levelPos, int &playerLayer, tson::Map &map);
	void loadObjects(tson::Layer &layer, int layerIndex, const glm::vec2 &levelPos) const;
	void processLayerModifications(tson::Layer &layer, GameEngine::ECS::Entity entity, const glm::vec2 &tileSize) const;

	int playerLayerIndex = 0;
	glm::vec2 playerSpawnPosition = {0, 0};
	GameEngine::ECS::Registry &registry;
	TextureManager &textureManager;
	InputManager &inputManager;
	AnimationFactory animationFactory{textureManager};
	const std::filesystem::path basePath = "assets/tiled/tilesets/";
	TileFactory tileFactory;
	GameEngine::ECS::EnemyFactory enemyFactory;
	PlayerFactory playerFactory;
	std::vector<std::unique_ptr<IObjectParser>> parsers;
	std::vector<EnemySpawner> &spawners;
	void registerParsers();
};
