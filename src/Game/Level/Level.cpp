#include "Level.hpp"
#include <iostream>
#include <filesystem>


Level::Level(TextureManager& textureManager, Renderer& renderer)
	: GameObject(renderer), textureManager(textureManager)
{
	dimensions = glm::ivec2(0, 0);
	//Load("assets/Maps/withtemplates.json");
}

void Level::Load(const std::string& levelFile, Player& player)
{
	this->player = &player;
	tson::Tileson t;
	map = t.parse(fs::path(levelFile));
	if (map->getStatus() != tson::ParseStatus::OK)
	{
		throw std::runtime_error("Failed to load map: " + map->getStatusMessage());
	}
	dimensions = glm::vec2(map->getSize().x, map->getSize().y);
	fs::path imagePath = map->getTilesets()[0].getFullImagePath();
	imagePath = "assets/tilesets" / imagePath.filename();
	tileSetTexture = textureManager.GetTexture(imagePath.string());

	//Load Background Textures

	tson::Layer* backgroundLayer = map->getLayer("Background");
	if (backgroundLayer && backgroundLayer->getType() == tson::LayerType::Group)
	{
		for (tson::Layer layer : backgroundLayer->getLayers()) {
			if (layer.getType() != tson::LayerType::ImageLayer) return;

			std::string layername = layer.getName();
			fs::path backgroundPath = layer.getImage();
			backgroundPath = "assets/background" / backgroundPath.filename();
			if (layername == "Background") {
				texture = textureManager.GetTexture(backgroundPath.string());
			}
			else if (layername == "Parallax") {
				//TODO: Support multiple parallax layers with different scroll factors
				parallaxTexture = textureManager.GetTexture(backgroundPath.string());
			}
			else if (layername == "Foreground") {
				foregroundTexture = textureManager.GetTexture(backgroundPath.string());
			}
		}
	}


	// Set player position to spawn point
	auto spawnPoint = map->getLayer("Entities")->getObjectsByName("Player");
	if (spawnPoint.size() != 1 && !spawnPoint[0].isPoint()) {
		throw std::runtime_error("Map must have exactly one Player spawn point");
	}
	const tson::Vector2 pos = spawnPoint[0].getPosition();

	player.position.x = pos.x;
	player.position.y = pos.y;
}

void Level::Update(const float deltaTime, const float stepSize) {
	//TODO: Move scroll factor to level file
	scrollPos += player->velocity.x * 0.12f * stepSize;
	SDL_Log("Scroll Pos: %f", scrollPos);
	if (scrollPos <= -texture->w) {
		scrollPos = 0;
	}
}

void Level::Render() {

	RenderBackground();
	RenderTiles();

}

void Level::RenderTiles() const
{
	for (const tson::Layer& layer : map->getLayers())
	{
		if (layer.getType() == tson::LayerType::TileLayer)
		{
			std::map<std::tuple<int, int>, tson::Tile*> tileData = layer.getTileData();
			for (const auto& [pos, tile] : tileData) {
				if (tile != nullptr) {
					glm::ivec2 position = glm::ivec2(std::get<0>(pos), std::get<1>(pos));
					glm::ivec2 tileSize = glm::ivec2(tile->getTileSize().x, tile->getTileSize().y);

					tson::Rect rectSrc = tile->getDrawingRect();

					renderer.DrawTexture(tileSetTexture.get(), rectSrc.x / rectSrc.width, rectSrc.y / rectSrc.height, position.x, position.y, spriteSize);
				}
			}

		}
	}
}

void Level::RenderBackground()
{
	renderer.DrawBackground(texture.get());
	renderer.DrawParalaxBackground(parallaxTexture.get(), scrollPos, 260, 1);
}

void Level::RenderForeground() const
{

}
