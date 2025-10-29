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

LevelFactory::LevelFactory(GameEngine::ECS::Registry &registry, TextureManager &textureManager)
    : registry(registry), textureManager(textureManager) {};

GameEngine::ECS::Entity LevelFactory::CreateLevel(tson::Map &map, const glm::vec2 &pos)
{
	std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();

	if (map.getStatus() != tson::ParseStatus::OK) {
		throw std::runtime_error("Failed to load map: " + map.getStatusMessage());
	}

	const auto level = registry.create();

	const glm::vec2 levelSize = glm::vec2(static_cast<float>(map.getSize().x * map.getTileSize().x),
	                                      static_cast<float>(map.getSize().y * map.getTileSize().y));
	registry.register_component<LevelComponent>(level, LevelComponent{.levelSize = levelSize, .levelName = levelFile});

	int layerIndex = 0;
	for (tson::Layer &layer : map.getLayers()) {
		LoadLayer(layer, layerIndex, pos);
	}

	std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - now).count();
	SDL_Log("Level loaded in %ld ms\n", duration);
	return level;
}

void LevelFactory::LoadLayer(tson::Layer &layer, int &layerIndex, const glm::vec2 &levelPos)
{
	switch (layer.getType()) {
	case tson::LayerType::ImageLayer:
		LoadBackground(layer, layerIndex, levelPos);
		break;
	case tson::LayerType::TileLayer:
		LoadTiles(layer, layerIndex, levelPos);
		break;
	case tson::LayerType::ObjectGroup:
		LoadObjects(layer, layerIndex, levelPos);
		break;
	case tson::LayerType::Group:
		for (auto &subLayer : layer.getLayers()) {
			LoadLayer(subLayer, layerIndex, levelPos);
		}
		break;
	default:
		break;
	}
	layerIndex++;
}

void LevelFactory::LoadTiles(const tson::Layer &layer, const int layerIndex, const glm::vec2 &levelPos)
{
	std::map<std::tuple<int, int>, tson::Tile *> tileData = layer.getTileData();
	for (const auto &[tilePos, tile] : tileData) {
		if (tile != nullptr) {
			const glm::ivec2 position = glm::ivec2(std::get<0>(tilePos), std::get<1>(tilePos));
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
			auto position_component = PositionComponent{position * tileSize};
			position_component.position += levelPos;
			registry.register_component(entity, position_component);

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

void LevelFactory::LoadBackground(tson::Layer &layer, const int layerIndex, const glm::vec2 &levelPos)
{
	std::string layername = layer.getName();
	fs::path backgroundPath = layer.getImage();
	backgroundPath = "assets/tiled/background" / backgroundPath.filename();
	if (layer.hasRepeatX()) {
		const float scrollSpeed = layer.getProp("scroll") ? layer.getProp("scroll")->getValue<float>() : 1;
		CreateBackground(backgroundPath, layerIndex, scrollSpeed, layer.getOffset().y+levelPos.y, true);
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

void LevelFactory::LoadObjects(tson::Layer &layer, int layerIndex, const glm::vec2 &levelPos)
{

	// Load all portals

	for (auto &obj : layer.getObjectsByType(tson::ObjectType::Rectangle)) {
		if (obj.getProperties().hasProperty("Portal")) {
			const auto portalProp = obj.getProp("Portal");
			auto portalClass = std::any_cast<tson::TiledClass>(portalProp->getValue());
			auto portalId = portalClass.get<std::string>("portalId");
			const auto targetLevel = portalClass.get<std::string>("targetLevel");
			const auto targetPortalId = portalClass.get<std::string>("targetPortalId");

			const auto entity = registry.create();
			auto position_component = PositionComponent{
			    .position = {static_cast<float>(obj.getPosition().x), static_cast<float>(obj.getPosition().y)}};
			position_component.position += levelPos;
			registry.register_component(entity, position_component);
			registry.register_component(
			    entity, InteractionComponent{.interactionBox{.x = 0,
			                                                 .y = 0,
			                                                 .w = static_cast<float>(obj.getSize().x),
			                                                 .h = static_cast<float>(obj.getSize().y)}});
			registry.register_component(
			    entity,
			    PortalComponent{.portalId = portalId, .targetLevel = targetLevel, .targetPortalId = targetPortalId});

			// if (spawnPoint == portalId) {
			// 	playerSpawnPosition = glm::vec2(static_cast<float>(obj.getPosition().x - obj.getSize().x / 2.0f),
			// 	                                static_cast<float>(obj.getPosition().y));
			// 	playerLayerIndex = layerIndex;
			// }
		}
	}
}
