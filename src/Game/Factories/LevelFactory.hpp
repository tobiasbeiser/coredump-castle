#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/Graphics/TextureManager.hpp"
#include <glm/glm.hpp>
#include <tileson.hpp>

class LevelFactory {
  public:
	LevelFactory(GameEngine::ECS::Registry &registry, TextureManager &textureManager, const std::string &levelFile,
	             const std::string &spawnPoint = "Player");
	void CreateLevel();

  private:
	void SpawnPlayer(tson::Layer &layer, const int layerIndex);
	void LoadTiles(const tson::Layer &layer, const int layerIndex);
	void LoadBackground(tson::Layer &layer, const int layerIndex);
	void CreateBackground(const fs::path &filename, const int layer, const float scrollSpeed = 0,
	                      const float yOffset = 0, const bool repeatX = false);
	void LoadLayer(tson::Layer &layer, int &layerIndex);
	void LoadObjects(tson::Layer &layer, int layerIndex);

	const std::string levelFile;
	const std::string spawnPoint;
	int playerLayerIndex = 0;
	glm::vec2 playerSpawnPosition = {0,0};
	GameEngine::ECS::Registry &registry;
	TextureManager &textureManager;
};
