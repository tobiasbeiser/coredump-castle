#include "WorldFactory.hpp"
#include "EnemyFactory.hpp"
#include "Engine/Debug/DebugSystem.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/Graphics/TextureManager.hpp"
#include "LevelFactory.hpp"
WorldFactory::WorldFactory(GameEngine::ECS::Registry &registry, const std::filesystem::path &projectPath,
                           TextureManager &textureManager, InputManager &inputManager, AudioManager &audioManager,
                           std::vector<EnemySpawner> &spawners)
    : registry(registry), project(tson::Project{projectPath}), textureManager(textureManager),
      inputManager(inputManager), audioManager(audioManager), spawners(spawners)
{
}
void WorldFactory::createWorld(const std::filesystem::path &worldPath)
{
	tson::World world;
	world.parse(worldPath);
	auto levelFactory = LevelFactory(registry, textureManager, inputManager, audioManager, spawners);

	for (const auto &data : world.getMapData()) {
		std::unique_ptr<tson::Map> map = tileson.parse(fs::path(world.getFolder() / data.fileName));
		map->getProperties().add("id", data.fileName, tson::Type::String);
		auto levelPos = glm::vec2(static_cast<float>(data.position.x), static_cast<float>(data.position.y));

		const float width = static_cast<float>(map->getSize().x * map->getTileSize().x);
		const float height = static_cast<float>(map->getSize().y * map->getTileSize().y);
		DebugSystem::instance().registerMap(data.fileName, SDL_FRect{levelPos.x, levelPos.y, width, height});

		GameEngine::ECS::Entity level = levelFactory.createLevel(*map, levelPos);
		registry.registerComponent<PositionComponent>(level, PositionComponent{.position = levelPos});
	}
}