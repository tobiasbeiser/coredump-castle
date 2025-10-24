#include "LevelFactory.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Render/BackgroundComponent.hpp"
#include "Engine/ECS/Components/Render/TextureComponent.hpp"
#include "Engine/ECS/Components/Render/TileComponent.hpp"
#include "Game/Factories/PlayerFactory.hpp"
#include <glm/vec2.hpp>

void LevelFactory::CreateLevel(const std::string &levelFile)
{
	tson::Tileson tileson;
	auto map = tileson.parse(fs::path(levelFile));
	if (map->getStatus() != tson::ParseStatus::OK) {
		throw std::runtime_error("Failed to load map: " + map->getStatusMessage());
	}

	int layerIndex = 0;
	for (tson::Layer &layer : map->getLayers()) {
		LoadLayer(layer, layerIndex);
	}
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
		SpawnPlayer(layer, layerIndex);
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
			const fs::path imagePath = "assets/tilesets" / tile->getTileset()->getFullImagePath().filename();

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
	backgroundPath = "assets/background" / backgroundPath.filename();
	SDL_Log("Loading background layer: %s from %s", layername.c_str(), backgroundPath.string().c_str());
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
	SDL_Log("Creating background: %s", filename.string().c_str());
	auto entity = registry.create();
	registry.register_component<TextureComponent>(entity,
	                                              TextureComponent{&textureManager.GetTexture(filename.string())});
	registry.register_component(
	    entity, BackgroundComponent{.scrollingSpeed = scrollSpeed, .yOffset = yOffset, .repeatX = repeatX});
	registry.register_component(entity, RenderComponent{.layer = layer, .flipX = false});
}

void LevelFactory::SpawnPlayer(tson::Layer &layer, const int layerIndex)
{
	// Set player position to spawn point
	const auto spawnPointFromMap = layer.getObjectsByName("Player");
	if (spawnPointFromMap.size() != 1 && !spawnPointFromMap.at(0).isPoint()) {
		throw std::runtime_error("Map must have exactly one Player spawn point");
	}
	const tson::Vector2 pos = spawnPointFromMap.at(0).getPosition();
	PlayerFactory::CreatePlayer(registry, textureManager, glm::vec2(pos.x, pos.y),
	                            std::filesystem::path("assets/player/char_blue_1.png"), layerIndex);
}
