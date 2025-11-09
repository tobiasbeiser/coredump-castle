#include "Engine/ECS/Registry.hpp"
#include "Engine/Graphics/TextureManager.hpp"
#include "Game/Factories/AnimationFactory.hpp"
#include <glm/glm.hpp>
#include <tileson.hpp>

class LevelFactory {
  public:
	LevelFactory(GameEngine::ECS::Registry &registry, TextureManager &textureManager);

	LevelFactory(LevelFactory &&) = delete;
	LevelFactory &operator=(LevelFactory &&) = delete;
	LevelFactory(const LevelFactory &) = delete;
	LevelFactory &operator=(const LevelFactory &) = delete;
	~LevelFactory() = default;

	GameEngine::ECS::Entity createLevel(tson::Map &map, const glm::vec2 &pos);

  private:
	void loadTiles(const tson::Layer &layer, int layerIndex, const glm::vec2 &levelPos) const;
	void loadBackground(const tson::Layer &layer, int layerIndex, const glm::vec2 &levelPos) const;
	void createBackground(const glm::vec2 &levelPos, const fs::path &filename, int layer, float scrollSpeed = 0,
	                      float yOffset = 0, bool repeatX = false) const;
	GameEngine::ECS::Entity createTile(int layerIndex, const glm::vec2 &levelPos, tson::Tile &tile, glm::vec2 position) const;
	void loadLayer(tson::Layer &layer, int &layerIndex, const glm::vec2 &levelPos, int &playerLayer);
	void loadObjects(tson::Layer &layer, int layerIndex, const glm::vec2 &levelPos) const;
	void loadPortals(const glm::vec2 &levelPos, tson::Object &obj) const;

	int playerLayerIndex = 0;
	glm::vec2 playerSpawnPosition = {0, 0};
	GameEngine::ECS::Registry &registry;
	TextureManager &textureManager;
	AnimationFactory animationFactory{registry, textureManager};
	const std::filesystem::path basePath = "assets/tiled/tilesets/";
};
