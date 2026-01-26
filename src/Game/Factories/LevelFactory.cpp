#include "LevelFactory.hpp"
#include "EnemyFactory.hpp"
#include "Engine/ECS/Components/Core/LevelComponent.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/LockComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Render/BackgroundComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/InactiveTag.hpp"
#include "Game/Factories/PlayerFactory.hpp"
#include "Parsers/EnemyParser.hpp"
#include "Parsers/KeyParser.hpp"
#include "Parsers/PlayerParser.hpp"
#include "Parsers/SpawnPointParser.hpp"
#include "Parsers/TooltipParser.hpp"

#include <glm/vec2.hpp>

LevelFactory::LevelFactory(GameEngine::ECS::Registry &registry, TextureManager &textureManager,
                           InputManager &inputManager, AudioManager &audioManager)
    : registry(registry), textureManager(textureManager), inputManager(inputManager),
      tileFactory(registry, textureManager, animationFactory), enemyFactory(registry, animationFactory),
      playerFactory(registry, textureManager, animationFactory, audioManager)
{
	registerParsers();
};

GameEngine::ECS::Entity LevelFactory::createLevel(tson::Map &map, const glm::vec2 &pos)
{
	animationFactory.setMap(map);
	tileFactory.setMap(map);
	enemyFactory.setMap(map);

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
		if (layer.getName() == "prototype") {
			continue;
		}
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
		loadObjects(layer, layerIndex, levelPos);
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

void LevelFactory::loadTiles(tson::Layer &layer, const int layerIndex, const glm::vec2 &levelPos) const
{
	std::map<std::tuple<int, int>, tson::Tile *> tileData = layer.getTileData();
	for (const auto &[tilePos, tile] : tileData) {
		if (tile != nullptr) {
			const auto position = glm::ivec2(std::get<0>(tilePos), std::get<1>(tilePos));
			const auto tileSize = glm::ivec2(tile->getTileSize().x, tile->getTileSize().y);
			const auto worldPosition = glm::vec2(position * tileSize) + levelPos;
			const GameEngine::ECS::Entity entity = tileFactory.createTile(*tile, worldPosition, layerIndex);
			processLayerModifications(layer, entity, glm::vec2(tileSize));
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

void LevelFactory::loadObjects(tson::Layer &layer, const int layerIndex, const glm::vec2 &levelPos) const
{
	for (auto &obj : layer.getObjects()) {
		bool parsed = false;
		for (const auto &parser : parsers) {
			if (parser->canParse(obj)) {
				parser->parse(obj, levelPos, layerIndex);
				parsed = true;
				break;
			}
		}
		if (!parsed) {
			tileFactory.createFromObject(obj, levelPos, layerIndex);
		}
	}
}
void LevelFactory::registerParsers()
{
	parsers.push_back(std::make_unique<PlayerParser>(playerFactory));
	parsers.push_back(std::make_unique<SpawnPointParser>());
	parsers.push_back(std::make_unique<EnemyParser>(enemyFactory));
	parsers.push_back(std::make_unique<SavePointParser>(registry, tileFactory));
	parsers.push_back(std::make_unique<KeyParser>(registry, tileFactory));
	parsers.push_back(std::make_unique<TooltipParser>(registry, inputManager));
}

void LevelFactory::processLayerModifications(tson::Layer &layer, GameEngine::ECS::Entity entity,
                                             const glm::vec2 &tileSize) const
{
	if (layer.getProperties().hasProperty("lock")) {
		const auto lock = LockComponent{layer.get<int>("lock")};
		if (!registry.has<CollisionComponent>(entity)) {
			registry.registerComponent(
			    entity, CollisionComponent{0, 0, static_cast<float>(tileSize.x), static_cast<float>(tileSize.y)});
		}
		registry.registerComponent(entity, lock);
	}
}
