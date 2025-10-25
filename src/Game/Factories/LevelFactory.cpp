#include "LevelFactory.hpp"
#include "Engine/ECS/Components/Core/LevelComponent.hpp"
#include "Engine/ECS/Components/Core/PortalComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InteractionComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Render/BackgroundComponent.hpp"
#include "Engine/ECS/Components/Render/TextureComponent.hpp"
#include "Engine/ECS/Components/Render/TileComponent.hpp"
#include "Game/Factories/PlayerFactory.hpp"
#include <glm/vec2.hpp>

LevelFactory::LevelFactory(GameEngine::ECS::Registry &registry, TextureManager &textureManager,
                           const std::string &levelFile, const std::string &spawnPoint)
    : registry(registry), textureManager(textureManager), levelFile(levelFile), spawnPoint(spawnPoint) {};

void LevelFactory::CreateLevel()
{
	std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();

	fs::path projectPath = fs::path("assets/tiled/CoredumpCastle.tiled-project");
	tson::Project project{projectPath};

	tson::Tileson tileson{&project};
	fs::path mapBasePath = projectPath.parent_path() / "maps";
	mapBasePath /= levelFile + ".json";
	auto map = tileson.parse(mapBasePath);
	if (map->getStatus() != tson::ParseStatus::OK) {
		throw std::runtime_error("Failed to load map: " + map->getStatusMessage());
	}

	auto level = registry.create();

	glm::vec2 levelSize = glm::vec2(static_cast<float>(map->getSize().x * map->getTileSize().x),
	                                static_cast<float>(map->getSize().y * map->getTileSize().y));
	registry.register_component<LevelComponent>(level, LevelComponent{.levelSize = levelSize, .levelName = levelFile});

	int layerIndex = 0;
	for (tson::Layer &layer : map->getLayers()) {
		LoadLayer(layer, layerIndex);
	}

	PlayerFactory::CreatePlayer(registry, textureManager, playerSpawnPosition,
	                            std::filesystem::path("assets/player/char_blue_1.png"), playerLayerIndex);

	std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - now).count();
	SDL_Log("Level loaded in %lld ms\n", duration);
}

void LevelFactory::LoadLayer(tson::Layer &layer, int &layerIndex)
{
	switch (layer.getType()) {
	case tson::LayerType::ImageLayer:
		LoadBackground(layer, layerIndex);
		break;
	case tson::LayerType::TileLayer:
		LoadTiles(layer, layerIndex);
		break;
	case tson::LayerType::ObjectGroup:
		LoadObjects(layer, layerIndex);
		break;
	case tson::LayerType::Group:
		for (auto &subLayer : layer.getLayers()) {
			LoadLayer(subLayer, layerIndex);
		}
		break;
	}
	layerIndex++;
}

void LevelFactory::LoadTiles(const tson::Layer &layer, const int layerIndex)
{
	std::map<std::tuple<int, int>, tson::Tile *> tileData = layer.getTileData();
	for (const auto &[pos, tile] : tileData) {
		if (tile != nullptr) {
			const glm::ivec2 position = glm::ivec2(std::get<0>(pos), std::get<1>(pos));
			const glm::ivec2 tileSize = glm::ivec2(tile->getTileSize().x, tile->getTileSize().y);
			const tson::Rect rectSrc = tile->getDrawingRect();
			const fs::path imagePath = "assets/tiled/tilesets" / tile->getTileset()->getFullImagePath().filename();

			auto entity = registry.create();
			registry.register_component(entity,
			                            TileComponent{.TileSize = static_cast<float>(tileSize.x),
			                                          .TileMapPosition = {(rectSrc.x / rectSrc.width) * tileSize.x,
			                                                              (rectSrc.y / rectSrc.height) * tileSize.y}});
			registry.register_component(entity, TextureComponent{&textureManager.GetTexture(imagePath)});
			registry.register_component(entity, RenderComponent{.layer = layerIndex});
			registry.register_component(entity, PositionComponent{position * tileSize});

			if (tile->getProp("collidable")) {
				auto collisionBoxes = tile->getObjectgroup().getObjectsByType(tson::ObjectType::Rectangle);
				if (!collisionBoxes.empty()) {
					registry.register_component(
					    entity, CollisionComponent{static_cast<float>(collisionBoxes.at(0).getPosition().x),
					                               static_cast<float>(collisionBoxes.at(0).getPosition().y),
					                               static_cast<float>(collisionBoxes.at(0).getSize().x),
					                               static_cast<float>(collisionBoxes.at(0).getSize().y)});
				} else {
					registry.register_component(entity, CollisionComponent{0, 0, static_cast<float>(tileSize.x),
					                                                       static_cast<float>(tileSize.y)});
				}
			}
		}
	}
}

void LevelFactory::LoadBackground(tson::Layer &layer, const int layerIndex)
{
	std::string layername = layer.getName();
	fs::path backgroundPath = layer.getImage();
	backgroundPath = "assets/tiled/background" / backgroundPath.filename();
	if (layer.hasRepeatX()) {
		const float scrollSpeed = layer.getProp("scroll") ? layer.getProp("scroll")->getValue<float>() : 1;
		CreateBackground(backgroundPath, layerIndex, scrollSpeed, layer.getOffset().y, true);
	} else {
		CreateBackground(backgroundPath, layerIndex);
	}
}

void LevelFactory::CreateBackground(const fs::path &filename, const int layer, const float scrollSpeed,
                                    const float yOffset, const bool repeatX)
{
	auto entity = registry.create();
	registry.register_component<TextureComponent>(entity,
	                                              TextureComponent{&textureManager.GetTexture(filename.string())});
	registry.register_component(
	    entity, BackgroundComponent{.scrollingSpeed = scrollSpeed, .yOffset = yOffset, .repeatX = repeatX});
	registry.register_component(entity, RenderComponent{.layer = layer, .flipX = false});
}

void LevelFactory::LoadObjects(tson::Layer &layer, int layerIndex)
{

	// Load all portals

	for (auto &obj : layer.getObjectsByType(tson::ObjectType::Rectangle)) {
		if (obj.getProperties().hasProperty("Portal")) {
			auto portalProp = obj.getProp("Portal");
			tson::TiledClass portalClass = std::any_cast<tson::TiledClass>(portalProp->getValue());
			std::string portalId = portalClass.get<std::string>("portalId");
			std::string targetLevel = portalClass.get<std::string>("targetLevel");
			std::string targetPortalId = portalClass.get<std::string>("targetPortalId");

			auto entity = registry.create();
			registry.register_component(entity,
			                            PositionComponent{.position = {static_cast<float>(obj.getPosition().x),
			                                                           static_cast<float>(obj.getPosition().y)}});
			registry.register_component(
			    entity, InteractionComponent{.interactionBox{.x = 0,
			                                                 .y = 0,
			                                                 .w = static_cast<float>(obj.getSize().x),
			                                                 .h = static_cast<float>(obj.getSize().y)}});
			registry.register_component(
			    entity,
			    PortalComponent{.portalId = portalId, .targetLevel = targetLevel, .targetPortalId = targetPortalId});

			if (spawnPoint == portalId) {
				playerSpawnPosition = glm::vec2(static_cast<float>(obj.getPosition().x - obj.getSize().x / 2.0f),
				                                static_cast<float>(obj.getPosition().y));
				playerLayerIndex = layerIndex;
			}
		}
	}

	if (spawnPoint == "Player") {
		const auto spawnPointFromMap = layer.getObjectsByName("Player");
		if (spawnPointFromMap.size() != 1 && !spawnPointFromMap.at(0).isPoint()) {
			throw std::runtime_error("Map must have exactly one Player spawn point");
		}
		const tson::Vector2 pos = spawnPointFromMap.at(0).getPosition();
		playerSpawnPosition = glm::vec2(pos.x, pos.y);
		playerLayerIndex = layerIndex;
	}
}
