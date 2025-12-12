#include "WorldFactory.hpp"
#include "EnemyFactory.hpp"
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

	for (const auto &data : world.getMapData()) {
		std::unique_ptr<tson::Map> map = tileson.parse(fs::path(world.getFolder() / data.fileName));
		map->getProperties().add("id", data.fileName, tson::Type::String);
		auto levelPos = glm::vec2(static_cast<float>(data.position.x), static_cast<float>(data.position.y));

		GameEngine::ECS::Entity level = levelFactory.createLevel(*map, levelPos);
		registry.registerComponent<PositionComponent>(level, PositionComponent{.position = levelPos});
	}
}