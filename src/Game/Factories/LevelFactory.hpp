#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/Graphics/TextureManager.hpp"

#include <tileson.hpp>

class LevelFactory {
  public:
	LevelFactory(GameEngine::ECS::Registry &registry, TextureManager &textureManager)
	    : registry(registry), textureManager(textureManager) {};
	void CreateLevel(const std::string &levelFile);

  private:
	void SpawnPlayer(tson::Layer &layer, const int layerIndex);
	void LoadTiles(const tson::Layer &layer, const int layerIndex);
	void LoadBackground(tson::Layer &layer, const int layerIndex);
	void CreateBackground(const fs::path &filename, const int layer, const float scrollSpeed = 0,
	                      const float yOffset = 0, const bool repeatX = false);
	void LoadLayer(tson::Layer &layer, int& layerIndex);

	GameEngine::ECS::Registry &registry;
	TextureManager &textureManager;
};
