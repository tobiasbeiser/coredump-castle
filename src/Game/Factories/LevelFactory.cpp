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

	LoadBackground(map);
	LoadTiles(map);

	SpawnPlayer(map);
}

void LevelFactory::LoadTiles(std::unique_ptr<tson::Map> &map)
{

	fs::path imagePath = map->getTilesets().at(0).getFullImagePath();
	imagePath = "assets/tilesets" / imagePath.filename();
	auto& tileSetTexture = textureManager.GetTexture(imagePath.string());

	for (const tson::Layer &layer : map->getLayers()) {
		
		if (layer.getType() == tson::LayerType::TileLayer) {
			Layer renderLayer = (layer.getName() == "Tiles") ? Layer::Tiles : Layer::Foreground;
			

			std::map<std::tuple<int, int>, tson::Tile *> tileData = layer.getTileData();
			for (const auto &[pos, tile] : tileData) {
				if (tile != nullptr) {
					const glm::ivec2 position = glm::ivec2(std::get<0>(pos), std::get<1>(pos));
					const glm::ivec2 tileSize = glm::ivec2(tile->getTileSize().x, tile->getTileSize().y);
					const tson::Rect rectSrc = tile->getDrawingRect();

					auto entity = registry.create();
					registry.register_component(
					    entity, TileComponent{.TileSize = static_cast<float>(tileSize.x),
					                          .TileMapPosition = {(rectSrc.x / rectSrc.width) * tileSize.x,
					                                              (rectSrc.y / rectSrc.height) * tileSize.y}});
					registry.register_component(entity, TextureComponent{&tileSetTexture});
					registry.register_component(entity, RenderComponent{.layer = renderLayer});
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
	}
}

void LevelFactory::LoadBackground(std::unique_ptr<tson::Map> &map)
{
	if (tson::Layer *backgroundLayer = map->getLayer("Background");
	    backgroundLayer && backgroundLayer->getType() == tson::LayerType::Group) {
		for (tson::Layer layer : backgroundLayer->getLayers()) {
			if (layer.getType() != tson::LayerType::ImageLayer)
				continue;

			std::string layername = layer.getName();
			fs::path backgroundPath = layer.getImage();
			const float scrollSpeed = layer.getProp("scroll") ? layer.getProp("scroll")->getValue<float>() : 1;

			backgroundPath = "assets/background" / backgroundPath.filename();
			if (layername == "Background") {
				CreateBackground(backgroundPath, Layer::Background);
			} else if (layername == "Parallax") {
				// TODO: Support multiple parallax layers with different scroll factors
				CreateBackground(backgroundPath, Layer::Parallax, scrollSpeed, layer.getOffset().y);
			} else if (layername == "Foreground") {
				 //CreateBackground(backgroundPath, Layer::Foreground);
			}
		}
	}
}

void LevelFactory::CreateBackground(const fs::path &filename, const Layer &layer, const float scrollSpeed,
                                    const float yOffset)
{
	auto entity = registry.create();
	registry.register_component<TextureComponent>(entity,
	                                              TextureComponent{&textureManager.GetTexture(filename.string())});
	registry.register_component(entity, BackgroundComponent{.scrollingSpeed = scrollSpeed, .yOffset = yOffset});
	registry.register_component(entity, RenderComponent{.layer = layer, .flipX = false});
}

void LevelFactory::SpawnPlayer(std::unique_ptr<tson::Map> &map)
{
	// Set player position to spawn point
	const auto spawnPointFromMap = map->getLayer("Entities")->getObjectsByName("Player");
	if (spawnPointFromMap.size() != 1 && !spawnPointFromMap.at(0).isPoint()) {
		throw std::runtime_error("Map must have exactly one Player spawn point");
	}
	const tson::Vector2 pos = spawnPointFromMap.at(0).getPosition();
	PlayerFactory::CreatePlayer(registry, textureManager, glm::vec2(pos.x, pos.y));
}
