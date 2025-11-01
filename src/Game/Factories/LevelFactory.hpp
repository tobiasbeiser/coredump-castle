#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/Graphics/TextureManager.hpp"
#include <glm/glm.hpp>
#include <tileson.hpp>

class LevelFactory {
  public:
	LevelFactory(GameEngine::ECS::Registry &registry, TextureManager &textureManager);
	GameEngine::ECS::Entity CreateLevel(tson::Map &map, const glm::vec2 &pos);

  private:
	void LoadTiles(const tson::Layer &layer, int layerIndex, const glm::vec2 &levelPos) const;
	void LoadAnimation(tson::Tile *const &tile, GameEngine::ECS::Entity entity) const;
	void LoadBackground(const tson::Layer &layer, int layerIndex, const glm::vec2 &levelPos) const;
	void CreateBackground(const fs::path &filename, int layer, float scrollSpeed = 0, float yOffset = 0,
	                      bool repeatX = false) const;
	void LoadLayer(tson::Layer &layer, int &layerIndex, const glm::vec2 &levelPos, int &playerLayer);
	void LoadObjects(tson::Layer &layer, int layerIndex, const glm::vec2 &levelPos) const;

	int levelId = 0;
	int playerLayerIndex = 0;
	glm::vec2 playerSpawnPosition = {0, 0};
	GameEngine::ECS::Registry &registry;
	TextureManager &textureManager;
	const std::filesystem::path basePath = "assets/tiled/tilesets/";
};
