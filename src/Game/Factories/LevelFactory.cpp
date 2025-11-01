#include "LevelFactory.hpp"
#include "Engine/ECS/Components/Core/LevelComponent.hpp"
#include "Engine/ECS/Components/Core/PortalComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InteractionComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Render/BackgroundComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/Components/Tags/InactiveTag.hpp"
#include "Game/Factories/PlayerFactory.hpp"
#include <glm/vec2.hpp>

LevelFactory::LevelFactory(GameEngine::ECS::Registry &registry, TextureManager &textureManager)
    : registry(registry), textureManager(textureManager) {};

GameEngine::ECS::Entity LevelFactory::CreateLevel(tson::Map &map, const glm::vec2 &pos)
{
	const std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();

	if (map.getStatus() != tson::ParseStatus::OK) {
		throw std::runtime_error("Failed to load map: " + map.getStatusMessage());
	}

	const auto level = registry.create();

	const glm::vec2 levelSize = glm::vec2(static_cast<float>(map.getSize().x * map.getTileSize().x),
	                                      static_cast<float>(map.getSize().y * map.getTileSize().y));

	int layerIndex = 0;
	int playerLayer = 0;
	for (tson::Layer &layer : map.getLayers()) {
		LoadLayer(layer, layerIndex, pos, playerLayer);
	}

	registry.register_component<LevelComponent>(
	    level, LevelComponent{.levelSize = levelSize, .levelId = levelId, .playerLayer = playerLayer});
	registry.register_component(level, PositionComponent{.position = pos});
	registry.register_component(level, InactiveTag{});

	const std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

	const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - now).count();
	SDL_Log("Level loaded in %ld ms\n", duration);

	levelId++;
	return level;
}

void LevelFactory::LoadLayer(tson::Layer &layer, int &layerIndex, const glm::vec2 &levelPos, int &playerLayer)
{
	switch (layer.getType()) {
	case tson::LayerType::ImageLayer:
		LoadBackground(layer, layerIndex, levelPos);
		break;
	case tson::LayerType::TileLayer:
		LoadTiles(layer, layerIndex, levelPos);
		break;
	case tson::LayerType::ObjectGroup:
		playerLayer = layerIndex;
		LoadObjects(layer, layerIndex, levelPos);
		break;
	case tson::LayerType::Group:
		for (auto &subLayer : layer.getLayers()) {
			LoadLayer(subLayer, layerIndex, levelPos, playerLayer);
		}
		break;
	default:
		break;
	}
	layerIndex++;
}

void LevelFactory::LoadTiles(const tson::Layer &layer, const int layerIndex, const glm::vec2 &levelPos) const
{
	std::map<std::tuple<int, int>, tson::Tile *> tileData = layer.getTileData();
	for (const auto &[tilePos, tile] : tileData) {
		if (tile != nullptr) {
			const glm::ivec2 position = glm::ivec2(std::get<0>(tilePos), std::get<1>(tilePos));
			const glm::ivec2 tileSize = glm::ivec2(tile->getTileSize().x, tile->getTileSize().y);
			glm::ivec2 imageSize = glm::ivec2(tile->getImageSize().x, tile->getImageSize().y);
			const glm::ivec2 offset =
			    glm::ivec2(tile->getTileset()->getTileOffset().x, tile->getTileset()->getTileOffset().y);
			const tson::Rect rectSrc = tile->getDrawingRect();

			if (imageSize.x == 0 && imageSize.y == 0) {
				imageSize = tileSize;
			}

			fs::path imagePath;
			if (tile->getTileset()->getType() == tson::TilesetType::ImageTileset) {
				imagePath = tile->getTileset()->getFullImagePath();
			} else {
				imagePath = basePath / tile->getImage();
			}

			const bool flipX = tile->getFlipFlags() == tson::TileFlipFlags::Vertically;
			const bool flipY = tile->getFlipFlags() == tson::TileFlipFlags::Horizontally;

			auto entity = registry.create();
			registry.register_component(
			    entity,
			    SpriteComponent{.texture = &textureManager.GetTexture(imagePath),
			                    .TileSize = {static_cast<float>(imageSize.x), static_cast<float>(imageSize.y)},
			                    .TileMapPosition = {static_cast<float>(rectSrc.x), static_cast<float>(rectSrc.y)}});
			registry.register_component(entity, RenderComponent{.layer = layerIndex, .flipX = flipX});
			auto position_component = PositionComponent{position * tileSize};
			position_component.position += offset;
			position_component.position += levelPos;
			registry.register_component(entity, position_component);

			if (tile->getAnimation().any()) {
				LoadAnimation(tile, entity);
			}

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

void LevelFactory::LoadAnimation(tson::Tile *const &tile, const GameEngine::ECS::Entity entity) const
{
	const auto imageSize = glm::ivec2(tile->getImageSize().x, tile->getImageSize().y);

	const auto &animation = tile->getAnimation();
	const auto framesCount = animation.size();
	const auto tileset = tile->getTileset();

	AnimationComponent animComp;
	animComp.animations.emplace_back(framesCount, framesCount * 0.07f); // textures have frame duration of 0.1s

	std::vector<AnimationFrame> frames;
	for (const auto frame : animation.getFrames()) {
		AnimationFrame animFrame{};
		animFrame.texture = &textureManager.GetTexture(basePath / tileset->getTile(frame.getTileId())->getImage());
		animFrame.TileMapPosition = glm::ivec2{0, 0};
		animFrame.TileSize = imageSize;
		frames.push_back(animFrame);
	}
	animComp.frames.push_back(frames);
	registry.register_component(entity, animComp);
}

void LevelFactory::LoadBackground(const tson::Layer &layer, const int layerIndex, const glm::vec2 &levelPos) const
{
	fs::path backgroundPath = layer.getImage();
	backgroundPath = "assets/tiled/background" / backgroundPath.filename();
	if (layer.hasRepeatX()) {
		const float scrollSpeed = layer.getParallax().x;
		CreateBackground(levelPos, backgroundPath, layerIndex, scrollSpeed, layer.getOffset().y, true);
	} else {
		CreateBackground(levelPos, backgroundPath, layerIndex);
	}
}

void LevelFactory::CreateBackground(const glm::vec2 &levelPos, const fs::path &filename, const int layer,
                                    const float scrollSpeed, const float yOffset, const bool repeatX) const
{
	const auto entity = registry.create();
	registry.register_component(entity, BackgroundComponent{
	                                        .texture = &textureManager.GetTexture(filename.string()),
	                                        .scrollingSpeed = scrollSpeed,
	                                        .yOffset = yOffset,
	                                        .repeatX = repeatX,
	                                        .levelId = levelId,
	                                    });
	registry.register_component(entity, RenderComponent{.layer = layer, .flipX = false});
	registry.register_component(entity, PositionComponent{.position = levelPos});
}

void LevelFactory::LoadObjects(tson::Layer &layer, int layerIndex, const glm::vec2 &levelPos) const
{
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
		}
	}
}
