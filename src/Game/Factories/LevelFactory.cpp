#include "LevelFactory.hpp"

#include "EnemyFactory.hpp"
#include "Engine/ECS/Components/Core/LevelComponent.hpp"
#include "Engine/ECS/Components/Core/PortalComponent.hpp"
#include "Engine/ECS/Components/Core/SavePointComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InteractionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/KeyComponent.hpp"
#include "Engine/ECS/Components/Gameplay/LockComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Render/BackgroundComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/Components/Tags/InactiveTag.hpp"
#include "Game/Factories/PlayerFactory.hpp"

#include <algorithm>
#include <cctype>
#include <glm/vec2.hpp>

LevelFactory::LevelFactory(GameEngine::ECS::Registry &registry, TextureManager &textureManager)
    : registry(registry), textureManager(textureManager) {};

GameEngine::ECS::Entity LevelFactory::createLevel(tson::Map &map, const glm::vec2 &pos)
{
	const std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();

	if (map.getStatus() != tson::ParseStatus::OK) {
		throw std::runtime_error("Failed to load map: " + map.getStatusMessage());
	}

	const auto level = registry.create();

	const glm::vec2 levelSize = glm::vec2(static_cast<float>(map.getSize().x * map.getTileSize().x),
	                                      static_cast<float>(map.getSize().y * map.getTileSize().y));

	int layerIndex = 0;
	constexpr int defaultPlayerLayer = 100;

	int playerLayer = defaultPlayerLayer;
	for (tson::Layer &layer : map.getLayers()) {
		loadLayer(layer, layerIndex, pos, playerLayer, map);
	}

	registry.registerComponent<LevelComponent>(level,
	                                           LevelComponent{.levelSize = levelSize, .playerLayer = playerLayer});
	registry.registerComponent(level, PositionComponent{.position = pos});
	registry.registerComponent(level, InactiveTag{});

	const std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

	const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - now).count();

	SDL_Log("Level loaded in %ld ms\n", static_cast<long>(duration));
	if (playerLayer == defaultPlayerLayer) {
		const auto mapname = map.getProp("id")->getValue<std::string>();
		SDL_Log("map (%s) does not define a player layer!", mapname.c_str());
	}

	return level;
}

void LevelFactory::loadLayer(tson::Layer &layer, int &layerIndex, const glm::vec2 &levelPos, int &playerLayer,
                             tson::Map &map)
{
	switch (layer.getType()) {
	case tson::LayerType::ImageLayer:
		loadBackground(layer, layerIndex, levelPos);
		break;
	case tson::LayerType::TileLayer:
		loadTiles(layer, layerIndex, levelPos);
		break;
	case tson::LayerType::ObjectGroup:
		if (layer.getProperties().hasProperty("player") && layer.getProp("player")->getValue<bool>()) {
			playerLayer = layerIndex;
		}
		loadObjects(layer, layerIndex, levelPos, map);
		break;
	case tson::LayerType::Group:
		for (auto &subLayer : layer.getLayers()) {
			loadLayer(subLayer, layerIndex, levelPos, playerLayer, map);
		}
		break;
	default:
		break;
	}
	layerIndex++;
}

GameEngine::ECS::Entity LevelFactory::createTile(const int layerIndex, const glm::vec2 &levelPos, tson::Tile &tile,
                                                 const glm::vec2 position) const
{
	const auto tileSize = glm::vec2(tile.getTileSize().x, tile.getTileSize().y);
	glm::ivec2 imageSize = glm::ivec2(tile.getImageSize().x, tile.getImageSize().y);
	const glm::ivec2 offset = glm::ivec2(tile.getTileset()->getTileOffset().x, tile.getTileset()->getTileOffset().y);
	const tson::Rect rectSrc = tile.getDrawingRect();

	if (imageSize.x == 0 && imageSize.y == 0) {
		imageSize = tileSize;
	}

	fs::path imagePath;
	if (tile.getTileset()->getType() == tson::TilesetType::ImageTileset) {
		imagePath = tile.getTileset()->getFullImagePath();
	} else {
		imagePath = basePath / tile.getImage();
	}

	const bool flipX = tile.getFlipFlags() == tson::TileFlipFlags::Horizontally;

	const auto entity = registry.create();
	registry.registerComponent(
	    entity, SpriteComponent{.texture = &textureManager.getTexture(imagePath),
	                            .tileSize = {static_cast<float>(imageSize.x), static_cast<float>(imageSize.y)},
	                            .tileMapPosition = {static_cast<float>(rectSrc.x), static_cast<float>(rectSrc.y)}});
	registry.registerComponent(entity, RenderComponent{.layer = layerIndex, .flipX = flipX});
	auto positionComponent = PositionComponent{position};
	positionComponent.position += offset;
	positionComponent.position += levelPos;
	positionComponent.previousPosition = positionComponent.position;
	registry.registerComponent(entity, positionComponent);

	if (tile.getAnimation().any()) {
		animationFactory.loadAnimation(&tile, entity);
	}

	if (tile.getProp("collidable")) {
		auto collisionBoxes = tile.getObjectgroup().getObjectsByType(tson::ObjectType::Rectangle);
		if (!collisionBoxes.empty()) {
			registry.registerComponent(entity,
			                           CollisionComponent{static_cast<float>(collisionBoxes.at(0).getPosition().x),
			                                              static_cast<float>(collisionBoxes.at(0).getPosition().y),
			                                              static_cast<float>(collisionBoxes.at(0).getSize().x),
			                                              static_cast<float>(collisionBoxes.at(0).getSize().y)});
		} else {
			registry.registerComponent(
			    entity, CollisionComponent{0, 0, static_cast<float>(tileSize.x), static_cast<float>(tileSize.y)});
		}
	}

	return entity;
}
void LevelFactory::loadTiles(tson::Layer &layer, const int layerIndex, const glm::vec2 &levelPos) const
{
	std::map<std::tuple<int, int>, tson::Tile *> tileData = layer.getTileData();
	for (const auto &[tilePos, tile] : tileData) {
		if (tile != nullptr) {
			const glm::ivec2 position = glm::ivec2(std::get<0>(tilePos), std::get<1>(tilePos));
			const glm::ivec2 tileSize = glm::ivec2(tile->getTileSize().x, tile->getTileSize().y);
			const GameEngine::ECS::Entity entity = createTile(layerIndex, levelPos, *tile, position * tileSize);
			if (layer.getProperties().hasProperty("lock")) {
				const auto lock = LockComponent{layer.get<int>("lock")};
				if (!registry.has<CollisionComponent>(entity)) {
					registry.registerComponent(entity, CollisionComponent{0, 0, static_cast<float>(tileSize.x),
					                                                      static_cast<float>(tileSize.y)});
				}
				registry.registerComponent(entity, lock);
			}
		}
	}
}

void LevelFactory::loadBackground(const tson::Layer &layer, const int layerIndex, const glm::vec2 &levelPos) const
{
	fs::path backgroundPath = layer.getImage();
	backgroundPath = "assets/tiled/background" / backgroundPath.filename();
	if (layer.hasRepeatX()) {
		const float scrollSpeed = layer.getParallax().x;
		createBackground(levelPos, backgroundPath, layerIndex, scrollSpeed, layer.getOffset().y, true);
	} else {
		createBackground(levelPos, backgroundPath, layerIndex);
	}
}

void LevelFactory::createBackground(const glm::vec2 &levelPos, const fs::path &filename, const int layer,
                                    const float scrollSpeed, const float yOffset, const bool repeatX) const
{
	const auto entity = registry.create();
	registry.registerComponent(entity, BackgroundComponent{.texture = &textureManager.getTexture(filename.string()),
	                                                       .scrollingSpeed = scrollSpeed,
	                                                       .yOffset = yOffset,
	                                                       .repeatX = repeatX});
	registry.registerComponent(entity, RenderComponent{.layer = layer, .flipX = false});
	registry.registerComponent(entity, PositionComponent{.position = levelPos});
}

void LevelFactory::loadObjects(tson::Layer &layer, const int layerIndex, const glm::vec2 &levelPos,
                               tson::Map &map) const
{

	for (auto &obj : layer.getObjectsByType(tson::ObjectType::Rectangle)) {
		if (obj.getProperties().hasProperty("Portal")) {
			loadPortals(levelPos, obj);
		}
	}
	for (auto &obj : layer.getObjects()) {
		if (obj.getName() == "Player") {
			PlayerFactory::createPlayer(registry, textureManager, layer.getObjectsByName("Player").front(), levelPos,
			                            map);
		}
		std::string name = obj.getName();
		std::ranges::transform(name, name.begin(), [](unsigned char c) { return std::tolower(c); });
		if (name.find("enemy") != std::string::npos) {
			EnemyFactory::createEnemy(registry, textureManager, obj, levelPos, map, layerIndex);
			continue;
		}
		if (const auto tileset = layer.getMap()->getTilesetByGid(obj.getGid()); tileset != nullptr) {
			const int tilesetGid = tileset->getFirstgid();
			if (const auto tile = tileset->getTile(obj.getGid() - tilesetGid + 1); tile != nullptr) {
				auto position = glm::vec2(obj.getPosition().x, obj.getPosition().y - obj.getSize().y);
				const GameEngine::ECS::Entity entity = createTile(layerIndex, levelPos, *tile, position);

				if (obj.getProp("key")) {
					const auto key = obj.get<int>("key");
					registry.registerComponent(entity, KeyComponent{key});
					registry.registerComponent(entity, InteractionComponent{0, 0, static_cast<float>(obj.getSize().x),
					                                                        static_cast<float>(obj.getSize().y)});
				}
				if (obj.getName() == "SavePoint") {
					registry.registerComponent(entity, SavePointComponent{});
					registry.registerComponent(entity, InteractionComponent{0, 0, static_cast<float>(obj.getSize().x),
					                                                        static_cast<float>(obj.getSize().y)});
				}
			}
		}
	}
}

void LevelFactory::loadPortals(const glm::vec2 &levelPos, tson::Object &obj) const
{
	const auto portalProp = obj.getProp("Portal");
	auto portalClass = std::any_cast<tson::TiledClass>(portalProp->getValue());
	auto portalId = portalClass.get<std::string>("portalId");
	const auto targetLevel = portalClass.get<std::string>("targetLevel");
	const auto targetPortalId = portalClass.get<std::string>("targetPortalId");

	const auto entity = registry.create();
	auto positionComponent = PositionComponent{
	    .position = {static_cast<float>(obj.getPosition().x), static_cast<float>(obj.getPosition().y)}};
	positionComponent.position += levelPos;
	registry.registerComponent(entity, positionComponent);
	registry.registerComponent(
	    entity,
	    InteractionComponent{.interactionBox{
	        .x = 0, .y = 0, .w = static_cast<float>(obj.getSize().x), .h = static_cast<float>(obj.getSize().y)}});
	registry.registerComponent(
	    entity, PortalComponent{.portalId = portalId, .targetLevel = targetLevel, .targetPortalId = targetPortalId});
}
