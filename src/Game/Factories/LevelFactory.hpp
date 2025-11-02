#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/Graphics/TextureManager.hpp"
#include <glm/glm.hpp>
#include <tileson.hpp>

class LevelFactory {
  public:
	LevelFactory(GameEngine::ECS::Registry &registry, TextureManager &texture_manager);

	LevelFactory(LevelFactory &&) = delete;
	LevelFactory &operator=(LevelFactory &&) = delete;
	LevelFactory(const LevelFactory &) = delete;
	LevelFactory &operator=(const LevelFactory &) = delete;
	~LevelFactory() = default;

	GameEngine::ECS::Entity CreateLevel(tson::Map &map, const glm::vec2 &pos);

  private:
	void LoadTiles(const tson::Layer &layer, int layerIndex, const glm::vec2 &levelPos) const;
	void LoadAnimation(tson::Tile *const &tile, GameEngine::ECS::Entity entity) const;
	void LoadBackground(const tson::Layer &layer, int layerIndex, const glm::vec2 &levelPos) const;
	void CreateBackground(const glm::vec2 &levelPos, const fs::path &filename, int layer, float scrollSpeed = 0,
	                      float yOffset = 0, bool repeatX = false) const;
	void CreateTile(int layerIndex, const glm::vec2 &levelPos, tson::Tile& tile, glm::vec2 position) const;
	void LoadLayer(tson::Layer &layer, int &layerIndex, const glm::vec2 &levelPos, int &playerLayer);
	void LoadObjects(tson::Layer &layer, int layerIndex, const glm::vec2 &levelPos) const;
	void LoadPortals(const glm::vec2 &levelPos, tson::Object &obj) const;


	int playerLayerIndex = 0;
	glm::vec2 playerSpawnPosition = {0, 0};
	GameEngine::ECS::Registry &registry;
	TextureManager &texture_manager;
	const std::filesystem::path basePath = "assets/tiled/tilesets/";
};
