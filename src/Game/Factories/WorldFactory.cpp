#include "WorldFactory.hpp"

#include "Engine/ECS/Components/Core/LevelComponent.hpp"
#include "Engine/Graphics/TextureManager.hpp"
#include "LevelFactory.hpp"
#include "PlayerFactory.hpp"
WorldFactory::WorldFactory(GameEngine::ECS::Registry &registry, const std::filesystem::path &projectPath,
                           TextureManager &textureManager)
    : registry(registry), project(tson::Project{projectPath}), textureManager(textureManager)
{
}
void WorldFactory::createWorld(const std::filesystem::path &worldPath)
{
	tson::World world;
	world.parse(worldPath);
	auto levelFactory = LevelFactory(registry, textureManager);
	glm::vec2 playerSpawnPosition{0, 0};
	int playerLayerIndex = 100;

	for (const auto &data : world.getMapData()) {
		std::unique_ptr<tson::Map> map = tileson.parse(fs::path(world.getFolder() / data.fileName));
		glm::vec2 levelPos = glm::vec2(static_cast<float>(data.position.x), static_cast<float>(data.position.y));

		for (auto layer : map->getLayers()) {
			if (layer.getType() == tson::LayerType::ObjectGroup && layer.getObjectsByName("Player").size() > 0) {
				const auto spawnPointFromMap = layer.getObjectsByName("Player");
				if (spawnPointFromMap.size() > 1) {
					throw std::runtime_error("Map must have no more than one Player spawn point");
				}
				const tson::Vector2 pos = spawnPointFromMap.at(0).getPosition();
				playerSpawnPosition = glm::vec2(pos.x, pos.y) + levelPos;
			}
		}
		GameEngine::ECS::Entity level = levelFactory.createLevel(*map, levelPos);
		registry.registerComponent<PositionComponent>(level, PositionComponent{.position = levelPos});
	}

	PlayerFactory::createPlayer(registry, textureManager, playerSpawnPosition,
	                            std::filesystem::path("assets/player/char_blue_1.png"), playerLayerIndex);
}