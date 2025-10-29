#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/Graphics/TextureManager.hpp"
#include <glm/glm.hpp>
#include <tileson.hpp>

class LevelFactory {
  public:
	LevelFactory(GameEngine::ECS::Registry &registry, TextureManager &textureManager);
	GameEngine::ECS::Entity CreateLevel(tson::Map& map, const glm::vec2& pos);

  private:
	void LoadTiles(const tson::Layer &layer, const int layerIndex, const glm::vec2& levelPos);
	void LoadBackground(tson::Layer &layer, const int layerIndex, const glm::vec2 &levelPos);
	void CreateBackground(const fs::path &filename, const int layer, const float scrollSpeed = 0,
	                      const float yOffset = 0, const bool repeatX = false);
	void LoadLayer(tson::Layer &layer, int &layerIndex, const glm::vec2 &levelPos);
	void LoadObjects(tson::Layer &layer, int layerIndex, const glm::vec2 &levelPos);

	const std::string levelFile;
	int playerLayerIndex = 0;
	glm::vec2 playerSpawnPosition = {0,0};
	GameEngine::ECS::Registry &registry;
	TextureManager &textureManager;
};
