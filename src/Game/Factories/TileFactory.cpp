#include "TileFactory.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Hazard/SpikeComponent.hpp"
#include "Engine/ECS/Components/Physics/AccelerationComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"

static const std::filesystem::path basePath = "assets/tiled/tilesets";

TileFactory::TileFactory(GameEngine::ECS::Registry &registry, TextureManager &textureManager,
                         AnimationFactory &animationFactory)
    : registry(registry), textureManager(textureManager), animationFactory(animationFactory)
{
}

GameEngine::ECS::Entity TileFactory::createTile(tson::Tile &tile, const glm::vec2 &worldPosition,
                                                const int layerIndex) const
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
	auto positionComponent = PositionComponent{worldPosition};
	positionComponent.position += offset;
	positionComponent.previousPosition = positionComponent.position;
	registry.registerComponent(entity, positionComponent);

	if (tile.getAnimation().any()) {
		animationFactory.loadAnimation(&tile, entity);
	}

	addProperties(entity, tile);

	return entity;
}

GameEngine::ECS::Entity TileFactory::createFromObject(const tson::Object &object, const glm::vec2 &levelPos,
                                                      const int layerIndex) const
{
	if (!map) {
		throw std::invalid_argument("TileFactory::createFromObject: map is null)");
	}

	auto *tileset = map->getTilesetByGid(object.getGid());

	if (tileset == nullptr) {
		return registry.create();
	}

	const auto tile = tileset->getTile(object.getGid() - tileset->getFirstgid() + 1);
	if (tile == nullptr) {
		return registry.create();
	}
	const auto position = glm::vec2(object.getPosition().x, object.getPosition().y - object.getSize().y);
	const auto worldPosition = position + levelPos;
	return createTile(*tile, worldPosition, layerIndex);
}
void TileFactory::setMap(tson::Map &tsonMap)
{
	map = &tsonMap;
}
void TileFactory::addProperties(const GameEngine::ECS::Entity entity, tson::Tile &tile) const
{
	addCollision(entity, tile);
	addHazard(entity, tile);
}
void TileFactory::addHazard(const GameEngine::ECS::Entity entity, tson::Tile &tile) const
{
	if (const auto hazardProp = tile.getProp("hazard"); hazardProp && hazardProp->getValue<bool>()) {
		registry.registerComponent<SpikeComponent>(entity, SpikeComponent{});
	}
}

void TileFactory::addCollision(const GameEngine::ECS::Entity entity, tson::Tile &tile) const
{

	if (const auto collProp = tile.getProp("collidable"); collProp && collProp->getValue<bool>()) {
		if (const auto collisionBoxes = tile.getObjectgroup().getObjectsByType(tson::ObjectType::Rectangle);
		    !collisionBoxes.empty()) {
			registry.registerComponent(entity,
			                           CollisionComponent{static_cast<float>(collisionBoxes.at(0).getPosition().x),
			                                              static_cast<float>(collisionBoxes.at(0).getPosition().y),
			                                              static_cast<float>(collisionBoxes.at(0).getSize().x),
			                                              static_cast<float>(collisionBoxes.at(0).getSize().y)});
		} else {
			registry.registerComponent(entity, CollisionComponent{0, 0, static_cast<float>(tile.getTileSize().x),
			                                                      static_cast<float>(tile.getTileSize().y)});
		}
	}
}